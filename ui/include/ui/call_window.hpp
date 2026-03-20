#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QResizeEvent>

class SIPClient;

class CallWindow : public QWidget {
    Q_OBJECT

public:
    explicit CallWindow(SIPClient& client, QWidget* parent = nullptr);

public slots:
    void on_call_state_changed(const QString& state,
                               const QString& call_id,
                               const QString& remote_uri);
    void on_incoming_call(const QString& call_id, const QString& caller);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
private slots:
    void on_call_clicked();
    void on_hangup_clicked();
    void on_answer_clicked();
    void on_reject_clicked();

private:
    void update_button_states();

    SIPClient& client_;

    QLineEdit*  callee_edit_;
    QPushButton* call_btn_;
    QPushButton* hangup_btn_;
    QPushButton* answer_btn_;
    QPushButton* reject_btn_;
    QLabel*      status_label_;
    QLabel*      call_id_label_;
};