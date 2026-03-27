/**
 * QuickJS / 现代浏览器通用 Fetch 深度集成测试
 */
async function runTests() {
    const LOG = {
        pass: (msg) => console.log(`  \x1b[32m[PASS]\x1b[0m ${msg}`),
        fail: (msg) => console.log(`  \x1b[31m[FAIL]\x1b[0m ${msg}`),
        info: (msg) => console.log(`  \x1b[34m[INFO]\x1b[0m ${msg}`),
        result: (p, f) => console.log(`\n\x1b[1m测试总结: \x1b[32m通过 ${p}\x1b[0m | \x1b[31m失败 ${f}\x1b[0m\n`)
    };

    // 简易断言库
    const expect = (actual) => ({
        toBe: (expected, msg) => {
            if (actual === expected) return true;
            throw new Error(`${msg}: 期望 [${expected}], 实际 [${actual}]`);
        },
        toInclude: (str, msg) => {
            if (actual.includes(str)) return true;
            throw new Error(`${msg}: 字符串未包含 [${str}]`);
        },
        toDeepEqual: (expected, msg) => {
            const dfs = (a, e) => {
                if (typeof a !== typeof e) return false;
                if (Array.isArray(a)) {
                    if (a.length !== e.length) return false;
                    for (let i = 0; i < a.length; i++) {
                        if (!dfs(a[i], e[i])) return false;
                    }
                    return true;
                }
                if (typeof a === 'object') {
                    const keys = Object.keys(a);
                    if (keys.length !== Object.keys(e).length) return false;
                    for (const key of keys) {
                        if (!dfs(a[key], e[key])) return false;
                    }
                    return true;
                }
                return a === e;
            };
            if (dfs(actual, expected)) return true;
            throw new Error(`${msg}: 数据不一致\n    实: ${actual}\n    期: ${expected}`);
        }
    });

    let passed = 0;
    let failed = 0;

    const tests = [
        {
            name: "GET 响应体与 Header 深度校验",
            fn: async () => {
                const res = await fetch('https://httpbin.org/get?name=qjs&id=123', {
                    headers: { 'X-Test-ID': '42' }
                });
                const data = await res.json();
                
                expect(res.status).toBe(200, "状态码异常");
                expect(data.args.name).toBe('qjs', "Query 参数 name 校验失败");
                expect(data.args.id).toBe('123', "Query 参数 id 校验失败");
                expect(data.headers['X-Test-Id']).toBe('42', "自定义请求头丢失");
                expect(res.headers.get('content-type')).toInclude('application/json', "响应头类型错误");
            }
        },
        {
            name: "POST JSON 结构完整性校验",
            fn: async () => {
                const payload = { nested: { a: 1 }, tags: [1, 2], str: "测试" };
                const res = await fetch('https://httpbin.org/post', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(payload)
                });
                const data = await res.json();
                
                expect(res.status).toBe(200, "状态码异常");
                expect(data.json).toDeepEqual(payload, "发送与接收的 JSON 不一致");
            }
        },
        {
            name: "Binary/ArrayBuffer 逐字节比对",
            fn: async () => {
                const raw = new Uint8Array([0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0xFF, 0x42, 0x24]);
                const res = await fetch('https://httpbin.org/post', {
                    method: 'POST',
                    body: raw.buffer
                });
                const data = await res.json();
                
                expect(data.data.length > 0).toBe(true, "二进制数据未发送成功");
                LOG.info(`服务器接收长度: ${data.data.length} 字符`);
            }
        },
        {
            name: "错误处理 (404 Not Found)",
            fn: async () => {
                const res = await fetch('https://httpbin.org/status/404');
                expect(res.ok).toBe(false, "res.ok 应该为 false");
                expect(res.status).toBe(404, "应该返回 404");
            }
        },
        {
            name: "Response 文本流转换校验",
            fn: async () => {
                const res = await fetch('https://httpbin.org/robots.txt');
                const text = await res.text();
                expect(text).toInclude('Disallow', "robots.txt 内容读取异常");
            }
        }
    ];


    for (const test of tests) {
        try {
            console.log(`\n\x1b[1m测试: ${test.name}\x1b[0m`);
            await test.fn();
            LOG.pass(test.name);
            passed++;
        } catch (e) {
            LOG.fail(`${test.name}\n      \x1b[33m原因: ${e.message}\x1b[0m`);
            failed++;
        }
    }

    LOG.result(passed, failed);
    return { passed, failed };
}

// 启动执行
await runTests();