#include "ui/call_window.hpp"
#include "client/sip_client.hpp"

#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QGuiApplication>
#include <QScreen>

CallWindow::CallWindow(SIPClient& client, QWidget* parent)
    : QWidget(parent)
    , client_(client)
{
    setWindowTitle("SIP Call");
    setMinimumWidth(320);
    resize(500, 320);

    callee_edit_  = new QLineEdit(this);
    callee_edit_->setPlaceholderText("user@domain");

    call_btn_    = new QPushButton("Call",   this);
    hangup_btn_  = new QPushButton("Hang Up", this);
    answer_btn_  = new QPushButton("Answer", this);
    reject_btn_  = new QPushButton("Reject", this);
    status_label_  = new QLabel("Idle", this);
    call_id_label_ = new QLabel("", this);

    call_btn_->setFixedWidth(100);
    hangup_btn_->setFixedWidth(100);
    answer_btn_->setFixedWidth(100);
    reject_btn_->setFixedWidth(100);

    auto* form = new QFormLayout();
    form->addRow("Callee:", callee_edit_);

    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();
    btnRow->addWidget(call_btn_);
    btnRow->addWidget(hangup_btn_);
    btnRow->addWidget(answer_btn_);
    btnRow->addWidget(reject_btn_);
    btnRow->addStretch();

    auto* centerCol = new QVBoxLayout();
    centerCol->addLayout(form);
    centerCol->addSpacing(12);
    centerCol->addLayout(btnRow);
    centerCol->addStretch();

    auto* topRow = new QHBoxLayout();
    topRow->addWidget(status_label_);
    topRow->addStretch();
    topRow->addWidget(call_id_label_);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(100, 40, 100, 40);
    root->addLayout(topRow);
    root->addLayout(centerCol);

    setLayout(root);

    connect(call_btn_,   &QPushButton::clicked, this, &CallWindow::on_call_clicked);
    connect(hangup_btn_, &QPushButton::clicked, this, &CallWindow::on_hangup_clicked);
    connect(answer_btn_, &QPushButton::clicked, this, &CallWindow::on_answer_clicked);
    connect(reject_btn_, &QPushButton::clicked, this, &CallWindow::on_reject_clicked);

    update_button_states();
}

void CallWindow::on_call_clicked()
{
    const QString callee = callee_edit_->text().trimmed();
    if (callee.isEmpty()) {
        QMessageBox::warning(this, "Input error", "Callee must not be empty.");
        return;
    }

    const int at = callee.indexOf('@');
    if (at < 1 || at == callee.size() - 1) {
        QMessageBox::warning(this, "Input error", "Callee must be in user@domain format.");
        return;
    }

    const std::string to_user   = callee.left(at).toStdString();
    const std::string to_domain = callee.mid(at + 1).toStdString();

    const std::string local = client_.state().registered_user();
    const auto sep = local.find('@');
    const std::string from_user   = local.substr(0, sep);
    const std::string from_domain = local.substr(sep + 1);

    client_.do_invite(from_user, from_domain, to_user, to_domain);
}

void CallWindow::on_hangup_clicked()
{
    client_.do_bye();
}

void CallWindow::on_answer_clicked()
{
    client_.do_answer();
}

void CallWindow::on_reject_clicked()
{
    client_.do_reject();
}



void CallWindow::on_call_state_changed(const QString& state,
                                        const QString& call_id,
                                        const QString& remote_uri)
{
    status_label_->setText(state);
    call_id_label_->setText(call_id.isEmpty() ? "" : "[" + call_id + "]");


    if (state == "Idle" || state == "Registered") {
        call_id_label_->clear();
    } else if (!remote_uri.isEmpty()) {

        QString display = remote_uri;
        if (display.startsWith("sip:", Qt::CaseInsensitive))
            display = display.mid(4);
        callee_edit_->setText(display);
    }

    update_button_states();
}

void CallWindow::on_incoming_call(const QString& call_id, const QString& caller)
{
    status_label_->setText("Incoming call from " + caller);
    call_id_label_->setText("[" + call_id + "]");
    update_button_states();
}



void CallWindow::update_button_states()
{
    const auto s = client_.state().get_state();
    using State = SIPClientStateManager::State;

    const bool idle     = (s == State::Registered);
    const bool busy     = (s == State::Calling || s == State::Ringing || s == State::InCall);
    const bool incoming = (s == State::Ringing) && !client_.state().active_call_id().empty();
    const bool outgoing = (s == State::Calling);

    call_btn_->setEnabled(idle);
    hangup_btn_->setEnabled(busy);
    answer_btn_->setEnabled(incoming && !outgoing);
    reject_btn_->setEnabled(incoming && !outgoing);
}

void CallWindow::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    int screenWidth = QGuiApplication::primaryScreen()->geometry().width();
    double ratio = qMin(0.35, 0.55 * width() / (double)screenWidth);
    int hMargin  = (int)(width()  * ratio);
    int vMargin  = (int)(height() * 0.13);
    layout()->setContentsMargins(hMargin, vMargin, hMargin, vMargin);
}
void CallWindow::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    update_button_states();
}