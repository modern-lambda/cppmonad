/**
* Copyright @CanftIn 2020
* LICENSE: GPL-v3.0
**/

#ifndef __CPPMONAD_H__
#define __CPPMONAD_H__

#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <boost/lexical_cast.hpp>
#include <string>
#include <numeric>
#include <iostream>
#include <vector>
#include <functional>
#include <list>
#include <map>


template <typename A>
using Vec = std::vector<A>;

template <typename A>
using List = std::list<A>;

template <typename A, typename B>
using Map = std::map<A, B>;

template <typename A>
using Maybe = boost::optional<A>;

template <typename A, typename B>
using Either = boost::variant<A, B>;

template <typename A, typename B>
using Function = std::function<B(A)>;

template <typename A>
using Endomorphism = Function<A const&, A>;

typedef std::string String;

template <typename T>
T id(T x) {
  return x;
}

// (b -> c) -> (a -> b) -> a -> c
template <typename A, typename B, typename C>
Function<A const&, C> compose(Function<B const&, C> f,
                              Function<A const&, B> g) {
  using namespace std::placeholders;
  return std::bind(f, std::bind(g, _1));
}


// class
template <typename FA>
class Functor;

// functions
// Functor a => (b -> c) -> a b -> a c
template <typename FA, typename A, typename B>
inline typename Functor<FA>::template F<B>
fmap(Function<A const&, B> const& f, FA const& fa) {
  return Functor<FA>::template fmap<B>(f, fa);
}

// instances
template <typename A>
class Functor<Vec<A>> {
public:
  template <typename B>
  using F = Vec<B>;

  template <typename B>
  static Vec<B> fmap(Function<A const&, B> const& f,
                     Vec<A> const& as) {
    Vec<B> bs;
    bs.reserve(as.size());
    std::back_insert_iterator<Vec<B>> bi(bs);
    std::transform(as.begin(), as.end(), bi, f);
    return bs;
  }
};

template <typename A>
class Functor<List<A>> {
public:
  template <typename B>
  using F = List<B>;

  template <typename B>
  static List<B> fmap(Function<A const&, B> const& f,
                      List<A> const& as) {
    List<B> bs;
    std::back_insert_iterator<List<B>> bi(bs);
    std::transform(as.begin(), as.end(), bi, f);
    return bs;
  }
};

template <typename K, typename A>
class Functor<Map<K, A>> {
public:
  template <typename B>
  using F = Map<K, B>;

  template <typename B>
  static Map<K, B> fmap(Function<A const&, B> const& f,
                        Map<K, A> const& as) {
    Map<K, B> bs;
    for (auto const& ka : as)
      bs.insert(ka.first, f(ka.second));
    return bs;
  }
};

template <typename A>
class Functor<Maybe<A>> {
public:
  template <typename B>
  using F = Maybe<B>;

  template <typename B>
  static F<B> fmap(Function<A const&, B> const& f,
                   Maybe<A> const& ma) {
    return ma ? f(*ma) : F<B>();
  }
};

template <typename A, typename B>
class Functor<Either<A, B>> {
public:
  template <typename C>
  using F = Either<A, C>;

  template <typename C>
  static F<C> fmap(Function<B const&, C> const& f,
                   Either<A, B> const& ab) {
    A const* a = boost::get<A>(ab);
    B const* b = boost::get<B>(ab);
    return a ? *a : f(*b);
  }
};

template <typename A, typename B>
class Functor<Function<A const&, B>> {
public:
  template <typename C>
  using F = Function<A const&, C>;

  template <typename C>
  static F<C> fmap(Function<B const&, C> const& f,
                   Function<A const&, B> const& ab) {
    return compose(f, ab);
  }
};


// class
template <typename A>
class Monoid;

// functions
template <typename A>
inline A empty() {
  return Monoid<A>::empty();
}

template <typename A>
inline A append(A const& x, A const& y) {
  return Monoid<A>::append(x, y);
}

template <typename A, typename I>
inline A concat(I b, I e) {
  return std::accumulate(b, e, empty<A>(), append<A>);
}

template <typename A, typename C>
inline A concat(C const& xs) {
  return concat<A>(xs.begin(), xs.end());
}

// instances
template <typename A>
class Monoid<Vec<A>> {
public:
  static Vec<A> empty() {
    return Vec<A>();
  }

  static Vec<A> append(Vec<A> const& x,
                       Vec<A> const& y) {
    Vec<A> res;
    res.reserve(x.size() + y.size());
    res.insert(res.end(), x.begin(), x.end());
    res.insert(res.end(), y.begin(), y.end());
    return res;
  }
};

template <typename A>
class Monoid<List<A>> {
public:
  static List<A> empty() {
    return List<A>();
  }

  static List<A> append(List<A> const& x,
                        List<A> const& y) {
    List<A> res;
    res.insert(res.end(), x.begin(), x.end());
    res.insert(res.end(), y.begin(), y.end());
    return res;
  }
};

template <typename A, typename B>
class Monoid<Map<A, B>> {
public:
  static Map<A, B> empty() {
    return Map<A, B>();
  }

  static Map<A, B> append(Map<A, B> const& x,
                          Map<A, B> const& y) {
    Map<A, B> res;
    res.insert(x.begin(), x.end());
    res.insert(y.begin(), y.end());
    return res;
  }
};

template <>
class Monoid<String> {
public:
  static String empty() {
    return String();
  }

  static String append(String const& a,
                       String const& b) {
    return a + b;
  }
};


// class
template <typename MA>
class Monad;

// Monad a => a -> m a
template <typename MA, typename A>
inline MA mreturn(A const& a) {
  return Monad<MA>::mreturn(a);
}

// Monad a => a b -> (b -> a c) -> a c
template <typename MA, typename MB, typename A>
inline MB operator >>= (MA const& ma,
                        Function<A const&, MB> const& f) {
  return Monad<MA>::bind(ma, f);
}

// Monad a => a b -> a c -> a c
template <typename MA, typename MB>
inline MB operator >> (MA const& ma, MB const& mb) {
  using A = typename Monad<MA>::Type;
  Function<A const&, MB> f = [=](A const&){ return mb; };
  return ma >>= f;
}

// instances
template <typename A>
class Monad<Vec<A>> {
public:
  typedef A Type;

  static Vec<A> mreturn(A const& a) {
    return Vec<A> { a };
  }

  template <typename B>
  static Vec<B> bind(Vec<A> const& as,
                     Function<A const&, Vec<B>> const& f) {
    Vec<B> bs;
    bs.reserve(as.size());
    for (auto const& a : as)
      for (auto const& b : f(a))
        bs.push_back(b);
    return bs;
  }
};

template <typename A>
class Monad<Maybe<A>> {
public:
  typedef A Type;

  static Maybe<A> mreturn(A const& a) {
    return Maybe<A>(a);
  }

  template <typename B>
  static Maybe<B> bind(Maybe<A> const& ma,
                       Function<A const&, Maybe<B>> const& f) {
    return ma ? f(*ma) : Maybe<B>();
  }
};

template <typename A, typename B>
class Monad<Either<A, B>> {
public:
  typedef B Type;

  static Either<A, B> mreturn(B const& b) {
    return Either<A, B>(b);
  }

  template <typename C>
  static Either<A, C> bind(Either<A, B> const& mb,
                           Function<B const&, Either<A, C>> const& f) {
    A const* a = boost::get<A>(&mb);
    B const* b = boost::get<B>(&mb);
    return a ? *a : f(*b);
  }
};

template <typename A, typename B>
class Monad<Function<A const&, B>> {
public:
  typedef B Type;

  static Function<A const&, B> mreturn(B const& b) {
    return [=](A const&){ return b; };
  }

  template <typename C>
  static Function<A const&, C> bind(Function<A const&, B> const& mb,
                                    Function<B const&,
                                             Function<A const&, C>> const& f) {
    return [=](A a){ return f(mb(a))(a); };
  }
};


// class
template <typename A>
class Show {
public:
  static String show(A const& x) {
    return boost::lexical_cast<String>(x);
  }
};

// functions
template <typename A>
inline String show(A const& x) {
  return Show<A>::show(x);
}

// instances
template <>
class Show<String> {
public:
  static String show(String const& s) {
    return '"' + s + '"';
  }
};

template <typename A>
class Show<Vec<A>> {
public:
  static String show(Vec<A> const& xs) {
    String res = "";
    for (auto const& x : xs)
      res += ", " + Show<A>::show(x);
    return "[" + res.erase(0, 2) + "]";
  }
};

template <typename A>
class Show<List<A>> {
public:
  static String show(List<A> const& xs) {
    String res = "";
    for (auto const& x : xs)
      res += ", " + Show<A>::show(x);
    return "[" + res.erase(0, 2) + "]";
  }
};

template <typename A, typename B>
class Show<Map<A, B>> {
public:
  static String show(List<A> const& xs) {
    String res = "";
    for (auto const& x : xs)
      res += ", " + Show<A>::show(x.first) + ": " + Show<B>::show(x.second);
    return "{" + res.erase(0, 2) + "}";
  }
};

template <typename A>
class Show<Maybe<A>> {
public:
  static String show(Maybe<A> const& x) {
    return x ? "Just " + Show<A>::Show::show(*x) : "Nothing";
  }
};

template <typename A, typename B>
class Show<Either<A, B>> {
public:
  static String show(Either<A, B> const& x) {
    return boost::apply_visitor(EitherShow(), x);
  }

private:
  class EitherShow : public boost::static_visitor<String> {
    public:
      String operator()(A const& a) const {
        return "Left " + Show<A>::show(a);
      }
      String operator()(B const& b) const {
        return "Right " + Show<B>::show(b);
      }
  };
};

#endif