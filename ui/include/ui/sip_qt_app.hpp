#pragma once
#include <QObject>
#include <QString>
#include <memory>

#include "ui/registration_window.hpp"

class SIPClient;


class SipQtApp : public QObject {
    Q_OBJECT

public:
    explicit SipQtApp(SIPClient& client, QObject* parent = nullptr);
    void show();


    void notify_register_response(bool success, const std::string& raw);

    signals:
        void register_response_received(bool success, const QString& raw_message);

private:
    RegistrationWindow window_;
};