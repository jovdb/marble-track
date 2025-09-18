/** A function with one argument */
export type OneArgFn<TIn, TOut> = (x: TIn) => TOut;

/** Execute a chain functions on a start value */
export function pipe<TIn, TOut, T1, T2, T3, T4, T5, T6, T7, T8, T9>(
  start: TIn,
  fn1: OneArgFn<TIn, T1>,
  fn2: OneArgFn<T1, T2>,
  fn3: OneArgFn<T2, T3>,
  fn4: OneArgFn<T3, T4>,
  fn5: OneArgFn<T4, T5>,
  fn6: OneArgFn<T5, T6>,
  fn7: OneArgFn<T6, T7>,
  fn8: OneArgFn<T7, T8>,
  fn9: OneArgFn<T8, T9>,
  fn10: OneArgFn<T9, TOut>
): TOut;
/** Execute a chain functions on a start value */
export function pipe<TIn, TOut, T1, T2, T3, T4, T5, T6, T7, T8>(
  start: TIn,
  fn1: OneArgFn<TIn, T1>,
  fn2: OneArgFn<T1, T2>,
  fn3: OneArgFn<T2, T3>,
  fn4: OneArgFn<T3, T4>,
  fn5: OneArgFn<T4, T5>,
  fn6: OneArgFn<T5, T6>,
  fn7: OneArgFn<T6, T7>,
  fn8: OneArgFn<T7, T8>,
  fn9: OneArgFn<T8, TOut>
): TOut;
/** Execute a chain functions on a start value */

export function pipe<TIn, TOut, T1, T2, T3, T4, T5, T6, T7>(
  start: TIn,
  fn1: OneArgFn<TIn, T1>,
  fn2: OneArgFn<T1, T2>,
  fn3: OneArgFn<T2, T3>,
  fn4: OneArgFn<T3, T4>,
  fn5: OneArgFn<T4, T5>,
  fn6: OneArgFn<T5, T6>,
  fn7: OneArgFn<T6, T7>,
  fn8: OneArgFn<T7, TOut>
): TOut;
/** Execute a chain functions on a start value */

export function pipe<TIn, TOut, T1, T2, T3, T4, T5, T6>(
  start: TIn,
  fn1: OneArgFn<TIn, T1>,
  fn2: OneArgFn<T1, T2>,
  fn3: OneArgFn<T2, T3>,
  fn4: OneArgFn<T3, T4>,
  fn5: OneArgFn<T4, T5>,
  fn6: OneArgFn<T5, T6>,
  fn7: OneArgFn<T6, TOut>
): TOut;
/** Execute a chain functions on a start value */

export function pipe<TIn, TOut, T1, T2, T3, T4, T5>(
  start: TIn,
  fn1: OneArgFn<TIn, T1>,
  fn2: OneArgFn<T1, T2>,
  fn3: OneArgFn<T2, T3>,
  fn4: OneArgFn<T3, T4>,
  fn5: OneArgFn<T4, T5>,
  fn6: OneArgFn<T5, TOut>
): TOut;
/** Execute a chain functions on a start value */

export function pipe<TIn, TOut, T1, T2, T3, T4>(
  start: TIn,
  fn1: OneArgFn<TIn, T1>,
  fn2: OneArgFn<T1, T2>,
  fn3: OneArgFn<T2, T3>,
  fn4: OneArgFn<T3, T4>,
  fn5: OneArgFn<T4, TOut>
): TOut;
/** Execute a chain functions on a start value */

export function pipe<TIn, TOut, T1, T2, T3>(
  start: TIn,
  fn1: OneArgFn<TIn, T1>,
  fn2: OneArgFn<T1, T2>,
  fn3: OneArgFn<T2, T3>,
  fn4: OneArgFn<T3, TOut>
): TOut;
/** Execute a chain functions on a start value */

export function pipe<TIn, TOut, T1, T2>(
  start: TIn,
  fn1: OneArgFn<TIn, T1>,
  fn2: OneArgFn<T1, T2>,
  fn3: OneArgFn<T2, TOut>
): TOut;
/** Execute a chain functions on a start value */
export function pipe<TIn, TOut, T1>(
  start: TIn,
  fn1: OneArgFn<TIn, T1>,
  fn2: OneArgFn<T1, TOut>
): TOut;
/** Execute a chain functions on a start value */
export function pipe<TIn, TOut>(start: TIn, fn1: OneArgFn<TIn, TOut>): TOut;

/**
 * Execute a chain functions on a start value:
 * The first argument is the start value, the next values are functions that are chained
 * - The start value is passed as input for the first function
 * - The output of the first function is passed as input for the second function
 * - The output of the second function is passed as input for the third function
 * - ...
 *
 * @example
 * pipe(10,
 * x => x + 1,  // or pass addOne function
 *x => x * 2,  // or pass double function
 * ) // Result: 42
 *
 * @example
 * // Object composition
 * pipe({x: 1},
 * obj => { obj["y"] = 2; return obj; }, // or pass addFeatureY function
 * obj => { obj["z"] = 3; return obj; }, // or pass addFeatureZ function
 * ) // Result: {x: 1, y: 2, z: 3}
 *
 * @example
 * // Function composition
 * pipe(() => 10,
 * fn => () => fn() + 1,
 * fn => () => fn() * 2,
 * ) // Result: () => 42
 *
 */
export function pipe<TIn>(start: TIn): TIn;
// eslint-disable-next-line @typescript-eslint/no-unsafe-function-type
export function pipe<TIn>(start: TIn, ...fns: Function[]) {
  // For better type inference, I included the start value as argument
  return fns.reduce((y, f) => f(y), start);
}
