#include "breeze-js/quickjs.h"
#include "breeze-js/quickjspp.hpp"
#include "breeze-js/script.h"

#include <functional>
#include <vector>

extern "C" {

int JS_EnqueueJob(JSContext *ctx, JSJobFunc *job_func, int argc,
                  JSValue *argv) {
  void *opaque = JS_GetContextOpaque(ctx);
  if (!opaque)
    return -1;
  auto &qjs_ctx = *static_cast<qjs::Context *>(opaque);
  auto *sctx = static_cast<breeze::script_context *>(qjs_ctx.script_ctx);
  if (!sctx)
    return -1;

  std::vector<JSValue> args(argc);
  for (int i = 0; i < argc; i++)
    args[i] = JS_DupValue(ctx, argv[i]);

  sctx->post([ctx, job_func, args = std::move(args)]() mutable {
    JSValue res = job_func(ctx, (int)args.size(), args.data());
    for (auto &a : args)
      JS_FreeValue(ctx, a);
    JS_FreeValue(ctx, res);
  });
  return 0;
}

JS_BOOL JS_IsJobPending(JSRuntime *rt) {
  auto *sctx =
      static_cast<breeze::script_context *>(JS_GetRuntimeOpaque(rt));
  if (!sctx)
    return 0;
  return sctx->task_queue_size.load(std::memory_order_relaxed) > 0;
}

} // extern "C"
