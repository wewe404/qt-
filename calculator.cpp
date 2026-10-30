#include "calculator.h"
#include "ui_calculator.h"

#include <QKeyEvent>
#include <QPushButton>
#include <cmath>

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

    // 所有按钮都走同一个槽：鼠标点击、键盘按键最终复用同一套处理逻辑。
    const auto buttons = findChildren<QPushButton *>();
    for (QPushButton *button : buttons) {
        connect(button, &QPushButton::clicked, this, &Calculator::onButtonClicked);
    }

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
    // 除零错误状态下，输入任何数字都视为重新开始
    if (m_errorState)
        handleClear();

    if (m_startNewInput) {
        // 开始输入新的操作数：直接覆盖显示值，并清掉上一次等号运算的回显
        m_currentInput = QString::number(digit);
        m_startNewInput = false;
    } else if (m_currentInput == QStringLiteral("0")) {
        // 前导 0 不保留，但 "0." 之后的 0 要保留
        if (!m_currentInput.contains(QLatin1Char('.')))
            m_currentInput = QString::number(digit);
    } else if (m_currentInput.size() < 16) {
        m_currentInput += QString::number(digit);
    }

    updateDisplay();
}

void Calculator::handleDecimal()
{
    if (m_errorState)
        handleClear();

    if (m_startNewInput) {
        // 直接输入小数点时补全为 "0."
        m_currentInput = QStringLiteral("0.");
        m_startNewInput = false;
    } else if (!m_currentInput.contains(QLatin1Char('.'))) {
        // 关键：同一个操作数只允许出现一个小数点，重复按 . 直接忽略
        m_currentInput += QLatin1Char('.');
    }

    updateDisplay();
}

void Calculator::handleOperator(const QString &op)
{
    if (m_errorState)
        return;

    if (m_hasFirstOperand && !m_pendingOperator.isEmpty()) {
        if (!m_startNewInput) {
            // 已有 “第一操作数 + 运算符 + 第二操作数” 的完整输入，
            // 再按运算符表示连续运算：先把前面的结果算出来。
            if (!applyOperation(m_pendingOperator, m_firstOperand, currentValue(), &m_firstOperand)) {
                showDivideByZeroError();
                return;
            }
            m_currentInput = numberText(m_firstOperand);
            m_pendingOperator.clear();
        }
        // 否则是连续按运算符（如 5 + *），只替换运算符，第一个操作数保持不变。
    } else {
        // 第一次按运算符：记录当前输入为第一个操作数
        m_firstOperand = currentValue();
        m_hasFirstOperand = true;
    }

    m_pendingOperator = op;
    m_startNewInput = true;   // 下一数字开始输入第二个操作数
    m_justEvaluated = false;
    m_lastExpression.clear();
    updateDisplay();
}

void Calculator::handleEquals()
{
    if (m_errorState)
        return;

    if (m_pendingOperator.isEmpty() || !m_hasFirstOperand) {
        // 没有待算的运算时，重复按 = 表示“用上次的运算符和操作数再算一次”
        if (m_justEvaluated && !m_lastOperator.isEmpty()) {
            const double left = currentValue();
            double result = 0.0;
            if (!applyOperation(m_lastOperator, left, m_lastOperand, &result)) {
                showDivideByZeroError();
                return;
            }
            m_currentInput = numberText(result);
            m_lastExpression = numberText(left) + QLatin1Char(' ')
                               + operatorText(m_lastOperator) + QLatin1Char(' ')
                               + numberText(m_lastOperand) + QStringLiteral(" =");
            updateDisplay();
        }
        return;
    }

    // 第二个操作数：若刚输入运算符还没输入数字，则取显示值（等同于重复第一个操作数）
    const double second = currentValue();
    const QString expression = numberText(m_firstOperand) + QLatin1Char(' ')
                               + operatorText(m_pendingOperator) + QLatin1Char(' ')
                               + numberText(second) + QStringLiteral(" =");

    double result = 0.0;
    if (!applyOperation(m_pendingOperator, m_firstOperand, second, &result)) {
        m_lastExpression = expression;
        showDivideByZeroError();
        return;
    }

    m_lastOperator = m_pendingOperator;
    m_lastOperand = second;
    m_currentInput = numberText(result);
    m_firstOperand = result;
    m_pendingOperator.clear();
    m_hasFirstOperand = false;
    m_startNewInput = true;
    m_justEvaluated = true;
    m_lastExpression = expression;
    updateDisplay();
}

void Calculator::handleBackspace()
{
    if (m_errorState) {
        handleClear();
        return;
    }

    // 刚运算完或刚清除时显示的并不是正在输入的数字，退格不生效
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
        if (right == 0.0) {
            return false;   // 除数为 0：交给调用方进入错误状态
        }
        *result = left / right;
    } else {
        return false;
    }

    if (std::isnan(*result) || std::isinf(*result))
        return false;
    return true;
}

void Calculator::showDivideByZeroError()
{
    m_errorState = true;
    m_currentInput = QStringLiteral("除数不能为 0");
    m_errorExpression = m_lastExpression.isEmpty() ? QString()
                                                   : QStringLiteral("%1 ÷ 0 =").arg(numberText(m_firstOperand));
    m_firstOperand = 0.0;
    m_pendingOperator.clear();
    m_hasFirstOperand = false;
    m_startNewInput = true;
    m_justEvaluated = false;
    m_lastExpression.clear();
    updateDisplay();
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
    if (m_errorState)
        return m_errorExpression;

    if (m_justEvaluated && !m_lastExpression.isEmpty())
        return m_lastExpression;

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
    // 'g' 会自动去掉多余的尾随 0，如 0.30000000000000004 显示为 0.3
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
