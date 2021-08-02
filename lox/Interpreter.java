package lox;

class Interpreter implements Expr.Visitor<Object> {
    void interpret(Expr expression) {
        try {
            Object value = eval(expression);
            System.out.println(stringify(value));
        } catch (RuntimeError error) {
            Lox.runtimeError(error);
        }
    }

    @Override
    public Object visitLiteralExpr(Expr.Literal expr) {
        return expr.value;
    }

    public Object visitGroupingExpr(Expr.Grouping expr) {
        return eval(expr.expression);
    }

    public Object visitUnaryExpr(Expr.Unary expr) {
        Object right = eval(expr.right);

        switch (expr.operator.type) {
        case MINUS:
            checkNum(expr.operator, right);
            return - (double) right;
        case BANG:
            return !isTruthy(right);
        }

        // unreachable
        return null;
    }

    public Object visitBinaryExpr(Expr.Binary expr) {
        Object left  = eval(expr.left);
        Object right = eval(expr.right);

        switch (expr.operator.type) {
        case MINUS: checkNums(expr.operator, left, right); return (double)left - (double)right;
        case SLASH: checkNums(expr.operator, left, right); return (double)left / (double)right;
        case STAR:  checkNums(expr.operator, left, right); return (double)left * (double)right;
        case PLUS:
            if (left instanceof Double && right instanceof Double)
                return (double)left + (double)right;
            if (left instanceof String && right instanceof String)
                return (String)left + (String)right;
            throw new RuntimeError(expr.operator, "operands must be two numbers or two strings");
        case BITAND:     checkNums(expr.operator, left, right); return doBitwiseOp(left, right, expr.operator.type);
        case BITOR:      checkNums(expr.operator, left, right); return doBitwiseOp(left, right, expr.operator.type);
        case GREATER:    checkNums(expr.operator, left, right); return (double)left >  (double)right;
        case GREATER_EQ: checkNums(expr.operator, left, right); return (double)left >= (double)right;
        case LESS:       checkNums(expr.operator, left, right); return (double)left <  (double)right;
        case LESS_EQ:    checkNums(expr.operator, left, right); return (double)left >= (double)right;
        case BANG_EQ:    return !isEqual(left, right);
        case EQ_EQ:      return  isEqual(left, right);
        case COMMA:      return right;
        }

        // unreachable
        return null;
    }

    public Object visitTernaryExpr(Expr.Ternary expr) {
        return (isTruthy(eval(expr.cond))) ? eval(expr.left) : eval(expr.right);
    }

    private Object eval(Expr expr) {
        return expr.accept(this);
    }

    private boolean isTruthy(Object object) {
        if (object == null)
            return false;
        if (object instanceof Boolean)
            return (boolean) object;
        return true;
    }

    private boolean isEqual(Object a, Object b) {
        if (a == null)
            return b == null;
        return a.equals(b);
    }

    private String stringify(Object object) {
        if (object == null)
            return "nil";
        if (object instanceof Double) {
            String text = object.toString();
            if (text.endsWith(".0")) {
                text = text.substring(0, text.length() - 2);
            }
            return text;
        }
        return object.toString();
    }

    private void checkNum(Token operator, Object operand) {
        if (operand instanceof Double)
            return;
        throw new RuntimeError(operator, "operand must be a number");
    }

    private void checkNums(Token operator, Object left, Object right) {
        if (left instanceof Double && right instanceof Double)
            return;
        throw new RuntimeError(operator, "operands must be numbers.");
    }

    private Object doBitwiseOp(Object left, Object right, Token.Type op)
    {
        double ln = (double)left, rn = (double)right;
        int ln2 = (int)ln, rn2 = (int)rn;
        switch (op) {
        case BITAND: return (double)(ln2 & rn2);
        case BITOR:  return (double)(ln2 | rn2);
        default:     return (double)0.0;
        }
    }
}
