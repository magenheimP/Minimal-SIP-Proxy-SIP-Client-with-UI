#include "ui/main_window.hpp"

#include "client/sip_client.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFrame>
#include <QMessageBox>
#include <QGuiApplication>
#include <QScreen>
#include <QResizeEvent>

static QFrame* make_section(QWidget* parent) {
    auto* f = new QFrame(parent);
    f->setFrameShape(QFrame::StyledPanel);
    f->setFrameShadow(QFrame::Plain);
    return f;
}

MainWindow::MainWindow(SIPClient& client, QWidget* parent)
    : QWidget(parent)
    , client_(client)
{
    setWindowTitle("SIP Client");
    setMinimumWidth(360);
    resize(520, 520);


    status_label_  = new QLabel("Not registered", this);
    call_id_label_ = new QLabel("", this);

    auto* statusRow = new QHBoxLayout();
    statusRow->addWidget(status_label_);
    statusRow->addStretch();
    statusRow->addWidget(call_id_label_);


    reg_section_   = make_section(this);
    username_edit_ = new QLineEdit(reg_section_);
    domain_edit_   = new QLineEdit(reg_section_);
    register_btn_  = new QPushButton("Register", reg_section_);
    register_btn_->setFixedWidth(110);


    auto* regForm = new QFormLayout();
    regForm->addRow("Username:", username_edit_);
    regForm->addRow("Domain:",   domain_edit_);

    auto* regBtnRow = new QHBoxLayout();
    regBtnRow->addStretch();
    regBtnRow->addWidget(register_btn_);


    auto* regLayout = new QVBoxLayout(reg_section_);
    regLayout->addLayout(regForm);
    regLayout->addSpacing(6);
    regLayout->addLayout(regBtnRow);


    call_section_ = make_section(this);
    callee_edit_  = new QLineEdit(call_section_);
    callee_edit_->setPlaceholderText("user@domain");
    call_btn_    = new QPushButton("Call",    call_section_);
    hangup_btn_  = new QPushButton("Hang Up", call_section_);
    answer_btn_  = new QPushButton("Answer",  call_section_);
    reject_btn_  = new QPushButton("Reject",  call_section_);

    for (auto* b : {call_btn_, hangup_btn_, answer_btn_, reject_btn_})
        b->setFixedWidth(90);

    auto* callForm = new QFormLayout();
    callForm->addRow("Callee:", callee_edit_);

    auto* callBtnRow = new QHBoxLayout();
    callBtnRow->addStretch();
    callBtnRow->addWidget(call_btn_);
    callBtnRow->addWidget(hangup_btn_);
    callBtnRow->addWidget(answer_btn_);
    callBtnRow->addWidget(reject_btn_);
    callBtnRow->addStretch();


    call_lock_overlay_ = new QWidget(call_section_);
    call_lock_overlay_->setStyleSheet(
        "background: rgba(128,128,128,0.08); border-radius: 6px;");
    call_lock_overlay_->setAttribute(Qt::WA_TransparentForMouseEvents);
    call_lock_overlay_->raise();


    auto* callLayout = new QVBoxLayout(call_section_);
    callLayout->addLayout(callForm);
    callLayout->addSpacing(6);
    callLayout->addLayout(callBtnRow);


    header_injection_ = new HeaderInjectionWidget(this);



    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(80, 32, 80, 32);
    root->setSpacing(12);
    root->addLayout(statusRow);
    root->addWidget(reg_section_);
    root->addWidget(call_section_);
    root->addWidget(header_injection_);
    root->addStretch();



    connect(register_btn_, &QPushButton::clicked, this, &MainWindow::on_register_clicked);
    connect(call_btn_,     &QPushButton::clicked, this, &MainWindow::on_call_clicked);
    connect(hangup_btn_,   &QPushButton::clicked, this, &MainWindow::on_hangup_clicked);
    connect(answer_btn_,   &QPushButton::clicked, this, &MainWindow::on_answer_clicked);
    connect(reject_btn_,   &QPushButton::clicked, this, &MainWindow::on_reject_clicked);

    update_button_states();
    set_call_section_locked(true);
}


void MainWindow::set_call_section_locked(bool locked)
{
    call_lock_overlay_->setVisible(locked);
    callee_edit_->setEnabled(!locked);
}

void MainWindow::update_button_states()
{
    using State = SIPClientStateManager::State;
    const auto s = client_.state().get_state();

    const bool idle     = (s == State::Registered);
    const bool in_call  = (s == State::InCall);
    const bool incoming = (s == State::Ringing)
                       && !client_.state().active_call_id().empty()
                       && client_.state().is_incoming_call();

    call_btn_->setEnabled(registered_ && idle);
    hangup_btn_->setEnabled(registered_ && in_call);
    answer_btn_->setEnabled(registered_ && incoming);
    reject_btn_->setEnabled(registered_ && incoming);
}



void MainWindow::on_register_clicked()
{
    const QString username = username_edit_->text().trimmed();
    const QString domain   = domain_edit_->text().trimmed();
    if (username.isEmpty() || domain.isEmpty()) {
        QMessageBox::warning(this, "Input error",
                             "Username and domain must not be empty.");
        return;
    }

    register_btn_->setEnabled(false);
    status_label_->setText("Registering…");

    const std::string extra = header_injection_->isEnabled()
        ? header_injection_->headersFor("REGISTER").toStdString()
        : std::string{};

    client_.do_register(username.toStdString(), domain.toStdString(), extra);
}


void MainWindow::on_register_response(bool success, const QString& raw_message)
{
    register_btn_->setEnabled(true);

    if (success) {
        registered_ = true;
        const QString user = username_edit_->text().trimmed()
                           + "@" + domain_edit_->text().trimmed();
        status_label_->setText("✓ Registered as " + user);
        set_call_section_locked(false);
        update_button_states();
        emit registration_succeeded();
    } else {
        status_label_->setText("✗ Registration failed");
        QMessageBox::information(this, "SIP Response", raw_message);
    }
}

void MainWindow::on_call_clicked()
{
    const QString callee = callee_edit_->text().trimmed();
    if (callee.isEmpty()) {
        QMessageBox::warning(this, "Input error", "Callee must not be empty.");
        return;
    }
    const int at = callee.indexOf('@');
    if (at < 1 || at == callee.size() - 1) {
        QMessageBox::warning(this, "Input error",
                             "Callee must be in user@domain format.");
        return;
    }

    const std::string to_user   = callee.left(at).toStdString();
    const std::string to_domain = callee.mid(at + 1).toStdString();

    const std::string local = client_.state().registered_user();
    const auto sep = local.find('@');
    const std::string from_user   = local.substr(0, sep);
    const std::string from_domain = local.substr(sep + 1);

    const bool inject = header_injection_->isEnabled();

    client_.set_pending_headers("INVITE",
        inject ? header_injection_->headersFor("INVITE").toStdString() : "");
    client_.set_pending_headers("ACK",
        inject ? header_injection_->headersFor("ACK").toStdString() : "");

    client_.do_invite(from_user, from_domain, to_user, to_domain);

    client_.do_invite(from_user, from_domain, to_user, to_domain);

}

void MainWindow::on_hangup_clicked()
{
    const std::string extra = header_injection_->isEnabled()
        ? header_injection_->headersFor("BYE").toStdString()
        : std::string{};

    client_.do_bye(extra);
}
void MainWindow::on_answer_clicked()  { client_.do_answer(); }
void MainWindow::on_reject_clicked()  { client_.do_reject(); }

void MainWindow::on_call_state_changed(const QString& state,
                                           const QString& call_id,
                                           const QString& remote_uri)
{
    if (registered_) {
        const QString user = username_edit_->text().trimmed()
                           + "@" + domain_edit_->text().trimmed();
        status_label_->setText(state == "Idle" || state == "Registered"
                                    ? "✓ Registered as " + user
                                    : state);
    }

    call_id_label_->setText(call_id.isEmpty() ? "" : "[" + call_id + "]");
    if (state == "Idle" || state == "Registered")
        call_id_label_->clear();

    if (!remote_uri.isEmpty()) {
        QString display = remote_uri;
        if (display.startsWith("sip:", Qt::CaseInsensitive))
            display = display.mid(4);
        callee_edit_->setText(display);
    }

    update_button_states();
}

void MainWindow::on_incoming_call(const QString& call_id,
                                      const QString& caller)
{
    status_label_->setText("Incoming call from " + caller);
    call_id_label_->setText("[" + call_id + "]");
    update_button_states();
}



void MainWindow::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);


    call_lock_overlay_->setGeometry(call_section_->rect());

    int screenWidth = QGuiApplication::primaryScreen()->geometry().width();
    double ratio = qMin(0.35, 0.55 * width() / (double)screenWidth);
    int hMargin  = (int)(width()  * ratio);
    int vMargin  = (int)(height() * 0.08);
    layout()->setContentsMargins(hMargin, vMargin, hMargin, vMargin);
}

void MainWindow::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    update_button_states();
}