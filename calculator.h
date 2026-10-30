#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <QMainWindow>
#include <functional>

QT_BEGIN_NAMESPACE
namespace Ui {
class Calculator;
}
QT_END_NAMESPACE

/**
 * @brief 参考 Windows 计算器布局的四则运算计算器（带键盘事件与一元函数）。
 *
 * 计算状态全部集中在 handle*() 成员函数中；鼠标按钮的 clicked 信号和键盘的
 * keyPressEvent() 都只负责把输入翻译成一次逻辑动作，再交给同一套函数处理，
 * 因此鼠标与键盘的行为完全一致。
 */
class Calculator : public QMainWindow
{
    Q_OBJECT

public:
    explicit Calculator(QWidget *parent = nullptr);
    ~Calculator() override;

    /** 显示区文本（当前输入、算式或结果），供自动化测试使用。 */
    QString displayText() const;

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    /** 所有按钮共用的点击槽，通过 sender() 的 objectName 分发。 */
    void onButtonClicked();

private:
    // ---- 计算器逻辑（鼠标与键盘共用的唯一入口） ----
    void handleDigit(int digit);
    void handleDecimal();
    void handleOperator(const QString &op);
    void handleEquals();
    void handleBackspace();
    void handleClear();
    void handleClearEntry();
    void handlePercent();
    void handleReciprocal();
    void handleSquare();
    void handleSquareRoot();
    void handleToggleSign();

    bool applyOperation(const QString &op, double left, double right, double *result) const;
    /** 对当前显示值执行一元运算（倒数、平方、开方等），异常结果进入错误状态。 */
    void applyUnaryOperation(const std::function<double(double)> &operation,
                             const QString &errorText);

    void showError(const QString &text);
    void resetState();
    void updateDisplay();
    QString buildDisplayText() const;
    double currentValue() const;

    static QString numberText(double value);
    static QString operatorText(const QString &op);

    Ui::Calculator *ui;

    // ---- 计算状态 ----
    double m_firstOperand = 0.0;     ///< 第一个操作数
    double m_lastOperand = 0.0;      ///< 上一次等号运算的第二个操作数（重复按 = 用）
    QString m_pendingOperator;       ///< 待执行的运算符（空表示没有）
    QString m_currentInput = "0";    ///< 当前正在输入的数字串 / 计算结果
    QString m_lastOperator;          ///< 上一次等号运算的运算符
    QString m_errorText;             ///< 错误提示文本

    bool m_startNewInput = true;     ///< 下一次数字输入是否开始一个新操作数
    bool m_hasFirstOperand = false;  ///< 是否已有第一个操作数
    bool m_justEvaluated = false;    ///< 是否刚完成一次等号运算
    bool m_errorState = false;       ///< 是否处于错误状态（除零、开方负数等）
};

#endif // CALCULATOR_H
