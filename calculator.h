#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class Calculator;
}
QT_END_NAMESPACE

class QKeyEvent;

/**
 * @brief 带键盘事件的四则运算计算器。
 *
 * 计算状态全部集中在 handleDigit()/handleDecimal()/handleOperator()/handleEquals()/
 * handleBackspace()/handleClear() 这几个成员函数中。鼠标按钮的 clicked 信号和
 * 键盘的 keyPressEvent() 都只负责把输入翻译成一次“逻辑动作”，再交给同一套函数处理，
 * 因此鼠标与键盘的行为天然保持一致。
 */
class Calculator : public QMainWindow
{
    Q_OBJECT

public:
    explicit Calculator(QWidget *parent = nullptr);
    ~Calculator() override;

    /** 主显示区（当前输入或计算结果）的文本，供自动化测试使用。 */
    QString displayText() const;
    /** 第二行表达式显示区的文本，供自动化测试使用。 */
    QString expressionText() const;

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    /** 所有数字/运算符/功能按钮共用的点击槽，通过 sender() 的 objectName 分发。 */
    void onButtonClicked();

private:
    // ---- 计算器逻辑（鼠标与键盘共用的唯一入口） ----
    void handleDigit(int digit);
    void handleDecimal();
    void handleOperator(const QString &op);
    void handleEquals();
    void handleBackspace();
    void handleClear();

    bool applyOperation(const QString &op, double left, double right, double *result) const;
    void showDivideByZeroError();
    void resetState();

    void updateDisplay();
    QString buildExpressionText() const;
    double currentValue() const;

    static QString numberText(double value);
    static QString operatorText(const QString &op);

    Ui::Calculator *ui;

    // ---- 计算状态 ----
    double m_firstOperand = 0.0;     ///< 第一个操作数
    double m_lastOperand = 0.0;      ///< 上一次等号运算的第二个操作数（重复按 = 用）
    QString m_pendingOperator;       ///< 待执行的运算符（空表示没有）
    QString m_currentInput = "0";    ///< 当前正在输入的数字串
    QString m_lastOperator;          ///< 上一次等号运算的运算符
    QString m_lastExpression;        ///< 上一次等号运算的完整算式（用于回显）
    QString m_errorExpression;       ///< 除零错误时回显的算式

    bool m_startNewInput = true;     ///< 下一次数字输入是否开始一个新操作数
    bool m_hasFirstOperand = false;  ///< 是否已有第一个操作数
    bool m_justEvaluated = false;    ///< 是否刚完成一次等号运算
    bool m_errorState = false;       ///< 是否处于除零错误状态
};

#endif // CALCULATOR_H
