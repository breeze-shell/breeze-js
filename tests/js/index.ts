import { runTests } from "./test"
// import "./tests/infra"
// import "./tests/filesystem"
import "./tests/webapi"

try {
    await runTests()
} catch (error) {
    console.error("An error occurred while running tests:", error, error.stack);
}