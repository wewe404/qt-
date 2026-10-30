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

/* ---------------- 按钮分发 ---------------- */

void Calculator::onButtonClicked()
{
    auto *button = qobject_cast<QPushButton *>(sender());
    if (!button)
        return;

    const QString name = button->objectName();

    if (name == QStringLiteral("equalsButton")) {
        handleEquals();
    } else if (name == QStringLiteral("clearButton")) {
        handleClear();
    } else if (name == QStringLiteral("backspaceButton")) {
        handleBackspace();
    } else if (name == QStringLiteral("decimalButton")) {
        handleDecimal();
    } else if (name == QStringLiteral("addButton")) {
        handleOperator(QStringLiteral("+"));
    } else if (name == QStringLiteral("subtractButton")) {
        handleOperator(QStringLiteral("-"));
    } else if (name == QStringLiteral("multiplyButton")) {
        handleOperator(QStringLiteral("*"));
    } else if (name == QStringLiteral("divideButton")) {
        handleOperator(QStringLiteral("/"));
    } else if (name.startsWith(QStringLiteral("digit")) && name.endsWith(QStringLiteral("Button"))) {
        bool ok = false;
        const int digit = name.mid(5, name.size() - 5 - 6).toInt(&ok);
        if (ok)
            handleDigit(digit);
    }
}

/* ---------------- 输入处理 ---------------- */

void Calculator::handleDigit(int digit)
{
    if (m_startNewInput) {
        m_currentInput = QString::number(digit);
        m_startNewInput = false;
    } else if (m_currentInput == QStringLiteral("0")) {
        m_currentInput = QString::number(digit);
    } else if (m_currentInput.size() < 16) {
        m_currentInput += QString::number(digit);
    }

    updateDisplay();
}

void Calculator::handleDecimal()
{
    if (m_startNewInput) {
        m_currentInput = QStringLiteral("0.");
        m_startNewInput = false;
    } else {
        m_currentInput += QLatin1Char('.');
    }

    updateDisplay();
}

void Calculator::handleOperator(const QString &op)
{
    if (m_hasFirstOperand && !m_pendingOperator.isEmpty()) {
        if (!m_startNewInput) {
            // 第二个操作数已输入，先完成前一步运算（连续运算）
            double result = 0.0;
            applyOperation(m_pendingOperator, m_firstOperand, currentValue(), &result);
            m_firstOperand = result;
            m_currentInput = numberText(result);
            m_pendingOperator.clear();
        }
    } else {
        m_firstOperand = currentValue();
        m_hasFirstOperand = true;
    }

    m_pendingOperator = op;
    m_startNewInput = true;
    updateDisplay();
}

void Calculator::handleEquals()
{
    if (m_pendingOperator.isEmpty() || !m_hasFirstOperand)
        return;

    const double second = currentValue();
    double result = 0.0;
    applyOperation(m_pendingOperator, m_firstOperand, second, &result);

    m_currentInput = numberText(result);
    m_firstOperand = result;
    m_pendingOperator.clear();
    m_hasFirstOperand = false;
    m_startNewInput = true;
    updateDisplay();
}

void Calculator::handleBackspace()
{
    if (m_startNewInput)
        return;

    m_currentInput.chop(1);
    if (m_currentInput.isEmpty() || m_currentInput == QStringLiteral("-"))
        m_currentInput = QStringLiteral("0");

    updateDisplay();
}

void Calculator::handleClear()
{
    resetState();
    updateDisplay();
}

/* ---------------- 运算与显示 ---------------- */

bool Calculator::applyOperation(const QString &op, double left, double right, double *result) const
{
    if (op == QStringLiteral("+")) {
        *result = left + right;
    } else if (op == QStringLiteral("-")) {
        *result = left - right;
    } else if (op == QStringLiteral("*")) {
        *result = left * right;
    } else if (op == QStringLiteral("/")) {
        if (right == 0.0)
            return false;
        *result = left / right;
    } else {
        return false;
    }
    return true;
}

void Calculator::showDivideByZeroError()
{
}

void Calculator::resetState()
{
    m_firstOperand = 0.0;
    m_lastOperand = 0.0;
    m_currentInput = QStringLiteral("0");
    m_pendingOperator.clear();
    m_lastOperator.clear();
    m_lastExpression.clear();
    m_errorExpression.clear();
    m_startNewInput = true;
    m_hasFirstOperand = false;
    m_justEvaluated = false;
    m_errorState = false;
}

void Calculator::updateDisplay()
{
    ui->display->setText(m_currentInput);
    ui->expressionDisplay->setText(buildExpressionText());
}

QString Calculator::buildExpressionText() const
{
    if (m_hasFirstOperand && !m_pendingOperator.isEmpty()) {
        QString text = numberText(m_firstOperand) + QLatin1Char(' ') + operatorText(m_pendingOperator);
        if (!m_startNewInput)
            text += QLatin1Char(' ') + m_currentInput;
        return text;
    }

    return QString();
}

double Calculator::currentValue() const
{
    bool ok = false;
    const double value = m_currentInput.toDouble(&ok);
    return ok ? value : 0.0;
}

QString Calculator::numberText(double value)
{
    QString text = QString::number(value, 'g', 15);
    if (text == QStringLiteral("-0"))
        text = QStringLiteral("0");
    return text;
}

QString Calculator::operatorText(const QString &op)
{
    if (op == QStringLiteral("+"))
        return QStringLiteral("+");
    if (op == QStringLiteral("-"))
        return QStringLiteral("−");
    if (op == QStringLiteral("*"))
        return QStringLiteral("×");
    if (op == QStringLiteral("/"))
        return QStringLiteral("÷");
    return op;
}
