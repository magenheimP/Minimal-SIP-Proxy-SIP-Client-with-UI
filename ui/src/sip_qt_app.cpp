#include "ui/sip_qt_app.hpp"
#include "ui/main_window.hpp"

SipQtApp::SipQtApp(SIPClient& client, QObject* parent)
    : QObject(parent)
    , window_(client)
{
    connect(this,     &SipQtApp::register_response_received,
            &window_, &MainWindow::on_register_response,
            Qt::QueuedConnection);

    connect(this,     &SipQtApp::call_state_changed,
            &window_, &MainWindow::on_call_state_changed,
            Qt::QueuedConnection);

    connect(this,     &SipQtApp::incoming_call_signal,
            &window_, &MainWindow::on_incoming_call,
            Qt::QueuedConnection);

    connect(this,     &SipQtApp::call_error_signal,
            &window_, &MainWindow::on_call_error,
            Qt::QueuedConnection);
}

void SipQtApp::show() { window_.show(); }

void SipQtApp::notify_register_response(bool success, const std::string& raw) {
    emit register_response_received(success, QString::fromStdString(raw));
}

void SipQtApp::notify_call_error(int code, const std::string& reason) {
    emit call_error_signal(code, QString::fromStdString(reason));
}
void SipQtApp::notify_call_state(const std::string& state,
                                  const std::string& call_id,
                                  const std::string& remote_uri) {
    emit call_state_changed(QString::fromStdString(state),
                            QString::fromStdString(call_id),
                            QString::fromStdString(remote_uri));
}

void SipQtApp::notify_incoming_call(const std::string& call_id,
                                     const std::string& caller) {
    emit incoming_call_signal(QString::fromStdString(call_id),
                              QString::fromStdString(caller));
}
