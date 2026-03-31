#include "ui/header_injection_widget.hpp"

const QStringList HeaderInjectionWidget::kMethods = {
    "REGISTER", "INVITE", "ACK", "BYE", "100", "180", "200"
};

HeaderInjectionWidget::HeaderInjectionWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(6);

    toggle_ = new QCheckBox("Inject custom SIP headers", this);
    root->addWidget(toggle_);

    tabs_ = new QTabWidget(this);
    tabs_->setVisible(false);
    buildTabs();
    root->addWidget(tabs_);


    tabs_->setMaximumHeight(0);

    setLayout(root);

    connect(toggle_, &QCheckBox::toggled,
            this,    &HeaderInjectionWidget::on_toggle);
    connect(toggle_, &QCheckBox::toggled,
            this,    &HeaderInjectionWidget::stateChanged);
}

void HeaderInjectionWidget::buildTabs()
{
    for (const QString& method : kMethods) {
        auto* editor = new QTextEdit();
        editor->setPlaceholderText(
            QString("Extra headers for %1 requests,\n"
                    "one per line\n"
                    "X-Custom-Header: value").arg(method));
        editor->setAcceptRichText(false);
        editor->setFixedHeight(110);

        connect(editor, &QTextEdit::textChanged,
                this,   &HeaderInjectionWidget::stateChanged);

        editors_[method] = editor;
        tabs_->addTab(editor, method);
    }
}

void HeaderInjectionWidget::on_toggle(bool checked)
{
    tabs_->setMaximumHeight(checked ? 16777215 : 0);
    tabs_->setVisible(checked);
}

void HeaderInjectionWidget::syncTo(HeaderInjectionWidget* other) const
{

    QSignalBlocker blocker(other);

    other->toggle_->setChecked(toggle_->isChecked());
    other->tabs_->setVisible(toggle_->isChecked());
    other->tabs_->setMaximumHeight(toggle_->isChecked() ? 16777215 : 0);

    for (const QString& method : kMethods) {
        if (editors_.contains(method) && other->editors_.contains(method))
            other->editors_[method]->setPlainText(editors_[method]->toPlainText());
    }
}

QString HeaderInjectionWidget::headersFor(const QString& method) const
{
    auto it = editors_.find(method);
    if (it == editors_.end()) return {};
    return it.value()->toPlainText().trimmed();
}

bool HeaderInjectionWidget::isEnabled() const
{
    return toggle_->isChecked();
}