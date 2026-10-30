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
    ui->mainLayout->setStretch(1, 1);

    // 所有按钮都走同一个槽：鼠标点击、键盘按键最终复用同一套处理逻辑。
    const auto buttons = findChildren<QPushButton *>();
    for (QPushButton *button : buttons) {
        button->setFocusPolicy(Qt::NoFocus);   // 避免按钮抢占键盘焦点
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

/* ---------------- 键盘事件 ---------------- */

void Calculator::keyPressEvent(QKeyEvent *event)
{
    const int key = event->key();

    // 数字键（主键盘与小键盘共用同一套 key 值）
    if (key >= Qt::Key_0 && key <= Qt::Key_9) {
        handleDigit(key - Qt::Key_0);
        return;
    }

    switch (key) {
    case Qt::Key_Period:
    case Qt::Key_Comma:
        handleDecimal();
        return;
    case Qt::Key_Plus:
        handleOperator(QStringLiteral("+"));
        return;
    case Qt::Key_Minus:
        handleOperator(QStringLiteral("-"));
        return;
    case Qt::Key_Asterisk:
        handleOperator(QStringLiteral("*"));
        return;
    case Qt::Key_Slash:
        handleOperator(QStringLiteral("/"));
        return;
    case Qt::Key_Percent:
        handlePercent();
        return;
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Equal:
        handleEquals();
        return;
    case Qt::Key_Backspace:
        handleBackspace();
        return;
    case Qt::Key_Escape:
        handleClear();
        return;
    case Qt::Key_Delete:
        handleClearEntry();
        return;
    case Qt::Key_F9:
        handleToggleSign();
        return;
    case Qt::Key_At:          // Shift + 2，对应 √x
        handleSquareRoot();
        return;
    case Qt::Key_R:           // 对应 1/x
        handleReciprocal();
        return;
    case Qt::Key_Q:           // 对应 x²
        handleSquare();
        return;
    default:
        break;
    }

    QMainWindow::keyPressEvent(event);
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
    } else if (name == QStringLiteral("clearEntryButton")) {
        handleClearEntry();
    } else if (name == QStringLiteral("backspaceButton")) {
        handleBackspace();
    } else if (name == QStringLiteral("decimalButton")) {
        handleDecimal();
    } else if (name == QStringLiteral("percentButton")) {
        handlePercent();
    } else if (name == QStringLiteral("reciprocalButton")) {
        handleReciprocal();
    } else if (name == QStringLiteral("squareButton")) {
        handleSquare();
    } else if (name == QStringLiteral("squareRootButton")) {
        handleSquareRoot();
    } else if (name == QStringLiteral("signButton")) {
        handleToggleSign();
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
    // 错误状态下，输入任何数字都视为重新开始
    if (m_errorState)
        handleClear();

    if (m_startNewInput) {
        // 开始输入新的操作数：覆盖显示值
        m_currentInput = QString::number(digit);
        m_startNewInput = false;
        m_justEvaluated = false;
    } else if (m_currentInput == QStringLiteral("0")) {
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
        m_justEvaluated = false;
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
            // 已有“第一操作数 运算符 第二操作数”，先结算前一步（连续运算）
            if (!applyOperation(m_pendingOperator, m_firstOperand, currentValue(), &m_firstOperand)) {
                showError(QStringLiteral("除数不能为 0"));
                return;
            }
            m_currentInput = numberText(m_firstOperand);
            m_pendingOperator.clear();
        }
        // 否则是连续按运算符（如 5 + ×），只替换运算符
    } else {
        m_firstOperand = currentValue();
        m_hasFirstOperand = true;
    }

    m_pendingOperator = op;
    m_startNewInput = true;      // 下一个数字开始输入第二个操作数
    m_justEvaluated = false;
    updateDisplay();
}

void Calculator::handleEquals()
{
    if (m_errorState)
        return;

    if (m_pendingOperator.isEmpty() || !m_hasFirstOperand) {
        // 没有待算的运算时，重复按 = 表示“用上次的运算符和操作数再算一次”
        if (m_justEvaluated && !m_lastOperator.isEmpty()) {
            double result = 0.0;
            if (!applyOperation(m_lastOperator, currentValue(), m_lastOperand, &result)) {
                showError(QStringLiteral("除数不能为 0"));
                return;
            }
            m_currentInput = numberText(result);
            m_justEvaluated = true;
            updateDisplay();
        }
        return;
    }

    // 第二个操作数：若刚输入运算符还没输入数字，则取显示值（等同于重复第一个操作数）
    const double second = currentValue();
    double result = 0.0;
    if (!applyOperation(m_pendingOperator, m_firstOperand, second, &result)) {
        showError(QStringLiteral("除数不能为 0"));
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

void Calculator::handleClearEntry()
{
    if (m_errorState) {
        resetState();
        updateDisplay();
        return;
    }

    // 仅清除当前输入，保留第一个操作数与待执行运算符
    m_currentInput = QStringLiteral("0");
    m_startNewInput = true;
    m_justEvaluated = false;
    updateDisplay();
}

/* ---------------- 一元运算 ---------------- */

void Calculator::handlePercent()
{
    applyUnaryOperation([](double value) { return value / 100.0; }, QString());
}

void Calculator::handleReciprocal()
{
    applyUnaryOperation([](double value) { return 1.0 / value; },
                        QStringLiteral("不能除以零"));
}

void Calculator::handleSquare()
{
    applyUnaryOperation([](double value) { return value * value; },
                        QStringLiteral("数值过大"));
}

void Calculator::handleSquareRoot()
{
    applyUnaryOperation([](double value) {
        return value < 0.0 ? std::nan("") : std::sqrt(value);
    }, QStringLiteral("无效输入"));
}

void Calculator::handleToggleSign()
{
    if (m_errorState)
        return;

    // 刚按完运算符、等待输入第二个操作数时，+/− 不生效
    if (m_startNewInput && m_hasFirstOperand && !m_pendingOperator.isEmpty())
        return;

    if (m_startNewInput) {
        // 结果或初始值：对显示值取反并作为新的输入
        m_currentInput = numberText(-currentValue());
        m_startNewInput = false;
    } else if (m_currentInput.startsWith(QLatin1Char('-'))) {
        m_currentInput.remove(0, 1);
    } else if (m_currentInput != QStringLiteral("0")) {
        m_currentInput.prepend(QLatin1Char('-'));
    }

    updateDisplay();
}

void Calculator::applyUnaryOperation(const std::function<double(double)> &operation,
                                     const QString &errorText)
{
    if (m_errorState)
        return;

    const double result = operation(currentValue());
    if (std::isnan(result) || std::isinf(result)) {
        showError(errorText.isEmpty() ? QStringLiteral("无效输入") : errorText);
        return;
    }

    m_currentInput = numberText(result);
    m_startNewInput = true;
    m_justEvaluated = false;
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
            return false;   // 除数为 0：交给调用方进入错误状态
        *result = left / right;
    } else {
        return false;
    }

    if (std::isnan(*result) || std::isinf(*result))
        return false;
    return true;
}

void Calculator::showError(const QString &text)
{
    m_errorState = true;
    m_errorText = text;
    m_firstOperand = 0.0;
    m_pendingOperator.clear();
    m_hasFirstOperand = false;
    m_startNewInput = true;
    m_justEvaluated = false;
    updateDisplay();
}

void Calculator::resetState()
{
    m_firstOperand = 0.0;
    m_lastOperand = 0.0;
    m_currentInput = QStringLiteral("0");
    m_pendingOperator.clear();
    m_lastOperator.clear();
    m_errorText.clear();
    m_startNewInput = true;
    m_hasFirstOperand = false;
    m_justEvaluated = false;
    m_errorState = false;
}

void Calculator::updateDisplay()
{
    ui->display->setText(buildDisplayText());
}

QString Calculator::buildDisplayText() const
{
    if (m_errorState)
        return m_errorText;

    // 刚按下运算符、等待第二个操作数时，显示 “第一操作数 运算符”
    if (m_hasFirstOperand && !m_pendingOperator.isEmpty() && m_startNewInput)
        return numberText(m_firstOperand) + QLatin1Char(' ') + operatorText(m_pendingOperator);

    return m_currentInput;
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
