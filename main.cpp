#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <vector>

#define GIVE
#define TAKE
#define KEEP

#define debug_assert assert

using namespace std;

extern "C" {
    const char* __asan_default_options() { return "detect_leaks=0"; }
};

// https://github.com/jorendorff/rust-grammar/blob/master/Rust.g4
// https://github.com/harpocrates/language-rust/blob/master/src/Language/Rust/Parser/Internal.y
using ll = long long;

struct String;
// L for location
struct Loc {
    const char *filename;
    ll si, line, col;
    Loc(const char *filename, ll si, ll line, ll col)
        : filename(filename), si(si), line(line), col(col){};
    Loc nextline() const { return Loc(filename, si + 1, line + 1, 1); }
    Loc next(char c) const {
        if (c == '\n') {
            return nextline();
        } else {
            return nextcol();
        }
    }

    Loc next(const String &s) const;

    Loc prev(char c) const {
        if (c == '\n') {
            assert(false && "don't know how to walk back newline");
        } else {
            return prevcol();
        }
    }

    Loc prev(const char *s) const {
        Loc l = *this;
        for (int i = strlen(s) - 1; i >= 0; --i) {
            l = l.prev(s[i]);
        }
        return l;
    }

    bool operator==(const Loc &other) const {
        return si == other.si && line == other.line && col == other.col;
    }

    bool operator!=(const Loc &other) const { return !(*this == other); }

   private:
    Loc nextcol() const { return Loc(filename, si + 1, line, col + 1); }
    Loc prevcol() const {
        assert(col - 1 >= 1);
        return Loc(filename, si - 1, line, col - 1);
    }
};

std::ostream &operator<<(std::ostream &o, const Loc &l) {
    return cout << ":" << l.line << ":" << l.col;
}

template <typename T>
struct Maybe {
    virtual T get() const { assert(false && "have no value!"); };
    virtual bool hasValue() const { return false; }
    operator bool() const { return hasValue(); }
};

template <typename T>
struct Just : Maybe<T> {
    Just(T t) : t(t) {}
    T get() const override { return t; }
    bool hasValue() const  override { return true; }

   private:
    T t;
};

// half open [...)
// substr := str[span.begin...span.end-1];
struct Span {
    Loc begin, end;
    Span(Loc begin, Loc end) : begin(begin), end(end) {
        debug_assert(end.si >= begin.si);
        debug_assert(!strcmp(begin.filename, end.filename));
    };
    ll nchars() const { return end.si - begin.si; }
};

std::ostream &operator<<(std::ostream &o, const Span &s) {
    return cout << s.begin << " - " << s.end;
}

// TODO: upgrade this to take a space, not just a location.
void vprintfspan(Span span, const char *raw_input, const char *fmt,
                 va_list args) {
    char *outstr = nullptr;
    vasprintf(&outstr, fmt, args);
    assert(outstr);
    cerr << "===\n";
    cerr << span.begin << ":" << span.end << "\n";

    cerr << "===\n";
    cerr << span << "\t" << outstr << "\n";
    for (ll i = span.begin.si; i < span.end.si; ++i) {
        cerr << raw_input[i];
    }
    cerr << "\n===\n";
}

void printfspan(Span span, const char *raw_input, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintfspan(span, raw_input, fmt, args);
    va_end(args);
}

void vprintferr(Loc loc, const char *raw_input, const char *fmt, va_list args) {
    char *outstr = nullptr;
    vasprintf(&outstr, fmt, args);
    assert(outstr);

    const int LINELEN = 80;
    const int CONTEXTLEN = 4;
    char line_buf[2*LINELEN];
    char pointer_buf[2*LINELEN];

    // find the previous newline character, or some number of characters back.
    ll nchars_back = 0;
    for (; loc.si - nchars_back >= 1 && raw_input[loc.si - nchars_back] != '\n'; nchars_back++)  {}

    int outix = 0;
    if (nchars_back > CONTEXTLEN) {
        nchars_back = CONTEXTLEN;
        for(int i = 0; i < 3; ++i, ++outix) { line_buf[outix] = '.'; pointer_buf[outix] = '.'; }
    }


    {
        int inix = loc.si-nchars_back;
        for (; inix-loc.si <= CONTEXTLEN; ++inix, ++outix) {
            if (raw_input[inix] == '\0') { break; }
            if (raw_input[inix] == '\n') { break; }
            cerr << "- [i: " << inix << "| " << raw_input[inix] << "]\n";
            line_buf[outix] = raw_input[inix];
            pointer_buf[outix] =  (inix == loc.si) ? '^' : ' ';
        }

        if (raw_input[inix] != '\0' && raw_input[inix] != '\n') {
            for(int i = 0; i < 3; ++i, ++outix) { line_buf[outix] = '.'; pointer_buf[outix] = '.'; }
        }
        line_buf[outix] = pointer_buf[outix] = '\0';
    }

    cerr << "\n==\n" << outstr << "\n" << loc.filename << loc << "\n" <<
        line_buf << "\n" << pointer_buf << "\n==\n";
    free(outstr);
}

void printferr(Loc loc, const char *raw_input, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintferr(loc, raw_input, fmt, args);
    va_end(args);
}

bool isWhitespace(char c) { return c == ' ' || c == '\n' || c == '\t'; }

struct String;
struct StringView {
    friend struct String;
    char operator[](int ix);
    ll nchars() { return sp.nchars(); }

   private:
    StringView(String &str, Span sp) : str(str), sp(sp){};
    String &str;
    Span sp;
};

// representation of string as str+number of characters.
struct String {
    const ll nchars;

    static String copyBuf(KEEP const char *buf, int len) {
        char *s = (char *)malloc(len+1);
        memcpy(s, buf, len);
        s[len] = 0;
        return String(s, len);
    }

    static String fromCStr(KEEP const char *str) {
        const ll len = strlen(str);
        char *out = (char *)malloc(len+1);
        memcpy(out, str, len);
        out[len] = 0;
        return String(out, len);
    }

    const char *asCStr() const { return str; }

    static String sprintf(const char *fmt, ...) {
        va_list args;
        va_start(args, fmt);
        return sprintf_(fmt, args);
        va_end(args);
    }

    char operator[](ll ix) const {
        assert(ix >= 0);
        assert(ix <= nchars);
        return str[ix];
    }

   private:
    // does not expect `buf` to be null-terminated.
    const char *str;
    String(TAKE const char *str, int nchars) : nchars(nchars), str(str) {}

    static String sprintf_(const char *fmt, va_list args) {
        char *str;
        ll len = vasprintf(&str, fmt, args);
        debug_assert(len == (ll)strlen(str));
        return String(str, len);
    }
};

char StringView::operator[](int ix) {
    debug_assert(ix < this->sp.nchars());
    return (this->str[ix]);
};

Loc Loc::next(const String &s) const {
    Loc cur = *this;
    for (int i = 0; i < s.nchars; ++i) {
        cur.next(s[i]);
    }
    return cur;
}

struct Error {
    String errmsg;
    Loc loc;
    Error(String errmsg, Loc loc) : errmsg(errmsg), loc(loc){};
};

struct Stream {
    Stream(const char *filename, const char *raw)
        : s(String::copyBuf(raw, strlen(raw))), l(filename, 0, 1, 1){};
    Span parseIdentifier();

    void parseLParen(){};
    void parseRParen() {}

    Maybe<Span> parseOptionalKeyword(const String keyword) {
        eatWhitespace();
        for (ll i = 0; i < keyword.nchars; ++i) {
            if (l.si + i >= s.nchars) {
                return Maybe<Span>();
            }
            if (s[l.si + i] != keyword[i]) {
                return Maybe<Span>();
            }
        }
        const Loc begin = l;
        l = l.next(keyword);
        return Just<Span>(Span(begin, l));
    };

    Maybe<Span> parseKeyword(const String keyword) {
        Maybe<Span> ms = parseOptionalKeyword(keyword);
        if (ms.hasValue()) {
            return ms;
        }

        addErr(Error(String::sprintf("expected |%s|", keyword.asCStr()), l));
        return Maybe<Span>();
    }

    void addErr(Error e) {
        errs.push_back(e);
        printferr(e.loc, s.asCStr(), e.errmsg.asCStr());
        assert(false && "error in parsing");
    }

    void addErrAtCurrentLoc(String err) { addErr(Error(err, l)); }

    bool eof() const { return l.si == s.nchars; }

   private:
    String s;
    Loc l;

    vector<Error> errs;

    void eatWhitespace() {
        while (l.si < s.nchars && isWhitespace(s[l.si])) { l.si++; }
    }
};

void parseUnion() {}
void parseStruct() {}
void parseFn(){};
void parsePubItem(){};

struct AST {};

AST *parseModule(Stream &s) { 
    while(!s.eof()) {
        if (s.parseOptionalKeyword(String::fromCStr("fn"))) {
        } else {
            s.addErrAtCurrentLoc(String::fromCStr("unknown top level starter"));
        }
    }
    return nullptr;
}

int main(int argc, const char *const *argv) {
    // assert_err(argc > 1 && "usage: %s <path-to-file>", argv[0]);
    assert(argc > 1 && "expected path to input file");
    FILE *f = fopen(argv[1], "rb");
    fseek(f, 0, SEEK_END); ll len = ftell(f); rewind(f);
    char *buf = (char *)malloc(len+1);
    ll nread = fread(buf, 1, len, f); assert(nread == len);
    buf[nread] = 0;

    Stream s(argv[1], buf);
    AST *ast = parseModule(s);
    return 0;
}

