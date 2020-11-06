# Lizzy: a lazy language frontend that uses [coremlir](https://github.com/bollu/coremlir)

- Syntax to be rust-like because I really believe it has the sanest syntax of
  any language i've ever used.
- What I really want checklist: 

1. Laziness. This is non-negotiable, since it gives so much niceness in terms
   of purity.
2. Performance. This seems to be at odds with Laziness. However, I believe that
   if we define our language semantics such that divergence is undefined behaviour,
   then we can recover lost performance, as we can equationally reason about
   strictness.
3. Nice-to-have: linearity/affine-types. [`carter`](http://www.wellposed.com/)
   has some interesting ideas of how we can use affine types to represent
   exhaustive cases in a pattern match, because it's _essentially_ (?) a form
   of separation logic. Would be super sexy to have, because then you can write
   code like this:

```hs
ManyCases = Case1 | Case2 | Case3

f :: NonExhaustive<ManyCases, Case1, Case2> -> Int
f Case1 = 1; f Case2 = 2

g :: NonExhaustive<ManyCases, Case3> -> Int
g Case3 = 43;

--  union the case analysis of f and g to get case analysis of h
h :: ManyCases -> Int -- h is exhaustive!
h = f | g  
```
