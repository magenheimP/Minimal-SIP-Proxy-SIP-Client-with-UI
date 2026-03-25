#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QStackedWidget>
#include "ui/header_injection_widget.hpp"

class SIPClient;

class MainWindow : public QWidget {
    Q_OBJECT

public:
    explicit MainWindow(SIPClient& client, QWidget* parent = nullptr);

    signals:
        void registration_succeeded();

public slots:
    void on_register_response(bool success, const QString& raw_message);
    void on_call_state_changed(const QString& state,
                               const QString& call_id,
                               const QString& remote_uri);
    void on_incoming_call(const QString& call_id, const QString& caller);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

private slots:
    void on_register_clicked();
    void on_call_clicked();
    void on_hangup_clicked();
    void on_answer_clicked();
    void on_reject_clicked();
    void on_headers_changed();

private:
    void update_button_states();
    void set_call_section_locked(bool locked);

    SIPClient&             client_;
    bool                   registered_ = false;


    QLabel*      status_label_;
    QLabel*      call_id_label_;

    QWidget*     reg_section_;
    QLineEdit*   username_edit_;
    QLineEdit*   domain_edit_;
    QPushButton* register_btn_;

    QWidget*     call_section_;
    QWidget*     call_lock_overlay_;
    QLineEdit*   callee_edit_;
    QPushButton* call_btn_;
    QPushButton* hangup_btn_;
    QPushButton* answer_btn_;
    QPushButton* reject_btn_;


    HeaderInjectionWidget* header_injection_;
};