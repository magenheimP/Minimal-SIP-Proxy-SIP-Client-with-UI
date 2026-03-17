#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

class SIPClient;

class RegistrationWindow : public QWidget {
    Q_OBJECT

public:
    explicit RegistrationWindow(SIPClient& client, QWidget* parent = nullptr);

public slots:
    void on_register_response(bool success, const QString& raw_message);

private slots:
    void on_register_clicked();

private:
    SIPClient&   client_;

    QLineEdit*   username_edit_;
    QLineEdit*   domain_edit_;
    QPushButton* register_btn_;
    QLabel*      status_label_;

    void resizeEvent(QResizeEvent* event) override;
};