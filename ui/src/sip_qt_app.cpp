#include "ui/sip_qt_app.hpp"

SipQtApp::SipQtApp(SIPClient& client, QObject* parent)
    : QObject(parent)
    , window_(client)
{

    connect(this,       &SipQtApp::register_response_received,
            &window_,   &RegistrationWindow::on_register_response,
            Qt::QueuedConnection);
}

void SipQtApp::show()
{
    window_.show();
}

void SipQtApp::notify_register_response(bool success, const std::string& raw)
{
    emit register_response_received(success, QString::fromStdString(raw));
}