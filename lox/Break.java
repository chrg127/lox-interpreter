package lox;

class Break extends RuntimeException {
    final Token keyword;

    Break(Token keyword) {
        super(null, null, false, false);
        this.keyword = keyword;
    }
}
