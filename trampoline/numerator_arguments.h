#ifndef NUMERATOR_H
#define NUMERATOR_H

template <typename ... Args>
struct Numerator;

template <>
struct Numerator<> {
    static const int INTEGER = 0;
    static const int SSE = 0;
};

template <typename First, typename ... Args>
struct Numerator<First, Args ...> {
    static const int INTEGER = Numerator<Args ...>::INTEGER + 1;
    static const int SSE = Numerator<Args ...>::SSE;
};

template <typename ... Args>
struct Numerator<float, Args ...> {
    static const int INTEGER = Numerator<Args ...>::INTEGER;
    static const int SSE = Numerator<Args ...>::SSE + 1;
};

template <typename ... Args>
struct Numerator<double, Args ...> {
    static const int INTEGER = Numerator<Args ...>::INTEGER;
    static const int SSE = Numerator<Args ...>::SSE + 1;
};

#endif // NUMERATOR_H
