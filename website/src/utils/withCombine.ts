export type FlattedPromise<T> = T extends Promise<Promise<infer U>> ? Promise<U> : T;

export interface ICombineCollector<TIn, TOut> {
  promise: Promise<{
    inputs: TIn[];
    output: TOut;
  }>;
  inputs: TIn[];
}

export const waitAsync = <T>(ms: number, value?: T) =>
  new Promise<T>((resolve) => {
    setTimeout(resolve, ms, value);
  });

/** Combine multiple calls to execute them as 1 at the next event loop.
	/*
	  combineInputs                      exec                 extractFromCombinedOutput
	┌──────────────┐              ┌──────────────────┐        ┌──────────────────┐              ┌──────────────┐
	│ INPUT1       │              │ INPUTS           │        │ OUTPUTS          │              │ OUTPUT1      │
	├──────────────┤ ───────────→ ├──────────────────┤ ─────→ ├──────────────────┤ ───────────→ ├──────────────┤
	│ [1, 2]       │    ┌───────→ │ [[1, 2], [3, 4]] │        │ [3, 7]           │ ───┐         │ 3            │
	└──────────────┘    │         └──────────────────┘        └──────────────────┘    │         └──────────────┘
						│                                                             │
	┌──────────────┐    │                                                             │         ┌──────────────┐
	│ INPUT2       │    │                                                             │         │ OUTPUT2      │
	├──────────────┤ ───┘                                                             └───────→ ├──────────────┤
	│ [3, 4]       │                                                                            │ 7            │
	└──────────────┘                                                                            └──────────────┘
	 */

export function withCombineAsync<TArgs extends any[], TResult>({
  collector = {},
  combineInputs,
  extractFromCombinedOutput,
  getDelayAsync = () => waitAsync(10),
}: {
  /**
   * An empty object that will collect all the inputs and the running timer
   * Store external so multiple combineAsync calls, can use the same collected information
   * Example: If a function create a new pipe every time, you must pass a collector, else a new collector is created per call -> so nothing can be collected
   * */
  collector: Partial<ICombineCollector<TArgs, FlattedPromise<TResult>>>;

  /**
   * Combine the array of function arguments to a new combined array of arguments,
   * The main function will be called with this returned list of this function.
   * @param inputs An array of all collected function arguments.
   */
  combineInputs(inputs: TArgs[]): TArgs;

  /**
   * Extract the part from the outputs for the right input
   * @param outputs The result of the combined function call
   * @param index The index of inputs that should be returned
   * @param inputs Collected array of inputs
   */
  extractFromCombinedOutput(
    outputs: FlattedPromise<TResult>,
    index: number,
    inputs: TArgs[]
  ): FlattedPromise<TResult>;
  getDelayAsync?(): Promise<any>;
}) {
  /** A empty object where we can keep information */
  return function addCombineAsync<TFn extends (...args: TArgs) => TResult>(fn: TFn): TFn {
    return function combine(...args: TArgs) {
      // Get index Counter
      if (!collector.inputs) collector.inputs = [];
      const index = collector.inputs.length;
      collector.inputs.push(args);

      // TODO: make it possible to start when array reaches an amount, Race with getDelayASync

      // Collect items during this period
      if (!collector.promise) {
        collector.promise = getDelayAsync() // Wait
          // Combine collected input, execute and return combined result
          .then(() => {
            const inputs = collector.inputs ? collector.inputs.slice(0) : [];
            const combinedInputs = combineInputs(inputs);

            collector.promise = undefined;
            collector.inputs = [];

            return {
              inputs,
              output: fn(...combinedInputs),
            };
          }) as any;
      }
      // Extract
      // If a shared collector is created by another combine function, it will wait for the result here
      return collector.promise!.then((e) => extractFromCombinedOutput(e.output, index, e.inputs));
    } as unknown as TFn;
  };
}
