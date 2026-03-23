#include "ui/registration_window.hpp"
#include "client/sip_client.hpp"

#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QGuiApplication>
#include <QScreen>

RegistrationWindow::RegistrationWindow(SIPClient& client, QWidget* parent)
    : QWidget(parent)
    , client_(client)
{
    setWindowTitle("SIP Registration");
    setMinimumWidth(320);
    resize(500, 300);

    username_edit_    = new QLineEdit(this);
    domain_edit_      = new QLineEdit(this);
    register_btn_     = new QPushButton("Register", this);
    status_label_     = new QLabel("Not registered", this);
    header_injection_ = new HeaderInjectionWidget(this);

    auto* form = new QFormLayout();
    form->addRow("Username:", username_edit_);
    form->addRow("Domain:",   domain_edit_);

    register_btn_->setFixedWidth(100);
    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();
    btnRow->addWidget(register_btn_);
    btnRow->addStretch();

    auto* centerCol = new QVBoxLayout();
    centerCol->addLayout(form);
    centerCol->addSpacing(8);
    centerCol->addWidget(header_injection_);
    centerCol->addSpacing(12);
    centerCol->addLayout(btnRow);
    centerCol->addStretch();

    auto* topRow = new QHBoxLayout();
    topRow->addWidget(status_label_);
    topRow->addStretch();

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(100, 40, 100, 40);
    root->addLayout(topRow);
    root->addLayout(centerCol);

    setLayout(root);

    connect(register_btn_, &QPushButton::clicked,
            this,           &RegistrationWindow::on_register_clicked);
}



void RegistrationWindow::on_register_clicked()
{
    const QString username = username_edit_->text().trimmed();
    const QString domain   = domain_edit_->text().trimmed();

    if (username.isEmpty() || domain.isEmpty()) {
        QMessageBox::warning(this, "Input error", "Username and domain must not be empty.");
        return;
    }

    register_btn_->setEnabled(false);
    status_label_->setText("Registering...");

    client_.do_register(username.toStdString(), domain.toStdString());
}

void RegistrationWindow::on_register_response(bool success, const QString& raw_message)
{
    register_btn_->setEnabled(true);
    if (success) {
        status_label_->setText("✓ Registered successfully");
        emit registration_succeeded();
    } else {
        status_label_->setText("✗ Registration failed");
        QMessageBox::information(this, "SIP Response", raw_message);
    }
}

void RegistrationWindow::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    int screenWidth = QGuiApplication::primaryScreen()->geometry().width();
    double ratio = qMin(0.35, 0.55 * width() / (double)screenWidth);
    int hMargin = (int)(width() * ratio);
    int vMargin = (int)(height() * 0.13);
    layout()->setContentsMargins(hMargin, vMargin, hMargin, vMargin);
}
