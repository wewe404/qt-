#include "calculator.h"
#include "ui_calculator.h"

#include <QPushButton>

Calculator::Calculator(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Calculator)
{
    ui->setupUi(this);
    setFocusPolicy(Qt::StrongFocus);

    // 显示区高度固定，剩余空间全部让给按键区
    ui->mainLayout->setStretch(0, 0);
    ui->mainLayout->setStretch(1, 0);
    ui->mainLayout->setStretch(2, 1);

    const auto buttons = findChildren<QPushButton *>();
    for (QPushButton *button : buttons)
        connect(button, &QPushButton::clicked, this, &Calculator::onButtonClicked);

    updateDisplay();
    setFocus();
}

Calculator::~Calculator()
{
    delete ui;
}

QString Calculator::displayText() const
{
    return ui->display->text();
}

QString Calculator::expressionText() const
{
    return ui->expressionDisplay->text();
}

/* ---------------- 按钮分发（功能待实现） ---------------- */

void Calculator::onButtonClicked()
{
    // TODO: 根据按钮的 objectName 分发到对应的处理函数
}

/* ---------------- 输入处理（占位实现） ---------------- */

void Calculator::handleDigit(int)
{
}

void Calculator::handleDecimal()
{
}

void Calculator::handleOperator(const QString &)
{
}

void Calculator::handleEquals()
{
}

void Calculator::handleBackspace()
{
}

void Calculator::handleClear()
{
}

bool Calculator::applyOperation(const QString &, double, double, double *) const
{
    return false;
}

void Calculator::showDivideByZeroError()
{
}

void Calculator::resetState()
{
}

void Calculator::updateDisplay()
{
    ui->display->setText(m_currentInput);
    ui->expressionDisplay->setText(buildExpressionText());
}

QString Calculator::buildExpressionText() const
{
    return QString();
}

double Calculator::currentValue() const
{
    bool ok = false;
    const double value = m_currentInput.toDouble(&ok);
    return ok ? value : 0.0;
}

QString Calculator::numberText(double)
{
    return QString();
}

QString Calculator::operatorText(const QString &)
{
    return QString();
}
