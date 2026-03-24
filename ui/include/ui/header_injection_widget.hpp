#pragma once
#include <QWidget>
#include <QCheckBox>
#include <QTabWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QMap>
#include <QString>

class HeaderInjectionWidget : public QWidget {
    Q_OBJECT

public:
    explicit HeaderInjectionWidget(QWidget* parent = nullptr);

    QString headersFor(const QString& method) const;
    bool isEnabled() const;


    void syncTo(HeaderInjectionWidget* other) const;

    signals:

        void stateChanged();

private slots:
    void on_toggle(bool checked);

private:
    void buildTabs();
    void connectEditorSignals();

    QCheckBox*  toggle_;
    QTabWidget* tabs_;

    static const QStringList kMethods;
    QMap<QString, QTextEdit*> editors_;
};