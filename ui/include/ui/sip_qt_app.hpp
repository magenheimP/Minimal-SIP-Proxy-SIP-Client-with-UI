#pragma once
#include <QObject>
#include <QString>
#include <string>

#include "main_window.hpp"


class SIPClient;

class SipQtApp : public QObject {
    Q_OBJECT

public:
    explicit SipQtApp(SIPClient& client, QObject* parent = nullptr);

    void show();
    void notify_register_response(bool success, const std::string& raw);
    void notify_call_state(const std::string& state,
                           const std::string& call_id,
                           const std::string& remote_uri);
    void notify_incoming_call(const std::string& call_id,
                              const std::string& caller);
    void notify_call_error(int code, const std::string& reason);

    signals:
        void register_response_received(bool success, const QString& raw);
    void call_error_signal(int code, const QString& reason);
    void call_state_changed(const QString& state,
                            const QString& call_id,
                            const QString& remote_uri);
    void incoming_call_signal(const QString& call_id, const QString& caller);

private:
    MainWindow window_;
};