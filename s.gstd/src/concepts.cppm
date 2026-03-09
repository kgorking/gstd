export module gs:concepts;
import std;

export template<typename From, typename To>
concept ConvertibleTo = requires { static_cast<To>(std::declval<From>()); };

export template <class _FTy, class... _ArgTys>
concept Predicate = std::invocable<_FTy, _ArgTys...> && ConvertibleTo<std::invoke_result_t<_FTy, _ArgTys...>, bool>;

export template<typename T>
concept Seq = requires(T t) {
    { t.next() } -> ConvertibleTo<bool>;
    { t.value() };
};

export template<typename T>
concept IterSeq = Seq<T> && requires(T t) {
    typename T::iterator;
    { t.begin() } -> std::same_as<typename T::iterator>;
    { t.end() } -> std::same_as<typename T::iterator>;
};
