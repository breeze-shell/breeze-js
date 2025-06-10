import { expect } from "chai";

// Simple ANSI coloring
const colors = {
  green: (text: string) => `\x1b[32m${text}\x1b[0m`,
  red: (text: string) => `\x1b[31m${text}\x1b[0m`,
  blue: (text: string) => `\x1b[34m${text}\x1b[0m`,
  cyan: (text: string) => `\x1b[36m${text}\x1b[0m`,
  gray: (text: string) => `\x1b[90m${text}\x1b[0m`,
};

type Test = {
  name: string;
  func: () => Promise<void> | void;
};

type DescribeBlock = {
  name: string;
  tests: Test[];
  children: DescribeBlock[];
  parent: DescribeBlock | null;
  beforeEachFunc?: () => Promise<void> | void;
  afterEachFunc?: () => Promise<void> | void;
};

const describeStack: DescribeBlock[] = [];
const rootDescribeBlock: DescribeBlock = { name: "Root", tests: [], children: [], parent: null };
let currentDescribeBlock: DescribeBlock = rootDescribeBlock;

let totalTests = 0;
let passedTests = 0;

export function describe(name: string, func: () => void) {
  const newDescribeBlock: DescribeBlock = { name, tests: [], children: [], parent: currentDescribeBlock };
  currentDescribeBlock.children.push(newDescribeBlock);
  describeStack.push(newDescribeBlock);
  currentDescribeBlock = newDescribeBlock;
  func();
  describeStack.pop();
  currentDescribeBlock = describeStack[describeStack.length - 1] || rootDescribeBlock;
}

export function it(name: string, func: () => Promise<void> | void) {
  if (!currentDescribeBlock) {
    throw new Error("it must be called within a describe block");
  }
  currentDescribeBlock.tests.push({ name, func });
}

export function beforeEach(func: () => Promise<void> | void) {
  if (!currentDescribeBlock) {
    throw new Error("beforeEach must be called within a describe block");
  }
  currentDescribeBlock.beforeEachFunc = func;
}

export function afterEach(func: () => Promise<void> | void) {
  if (!currentDescribeBlock) {
    throw new Error("afterEach must be called within a describe block");
  }
  currentDescribeBlock.afterEachFunc = func;
}

async function runDescribeBlock(describeBlock: DescribeBlock, indent: string = ""): Promise<boolean> {
  if (describeBlock.name !== "Root") {
    console.log(colors.blue(`${indent}${describeBlock.name}`));
  }

  let allTestsPassedInBlock = true;

  for (const test of describeBlock.tests) {
    totalTests++;
    try {
      if (describeBlock.beforeEachFunc) {
        await describeBlock.beforeEachFunc();
      }
      await test.func();
      console.log(colors.green(`${indent}  ✓ ${test.name}`));
      passedTests++;
    } catch (error: any) {
      console.error(colors.red(`${indent}  ✗ ${test.name}`));
      console.error(colors.gray(`${indent}    ${error.message}`));
      allTestsPassedInBlock = false;
    } finally {
      if (describeBlock.afterEachFunc) {
        await describeBlock.afterEachFunc();
      }
    }
  }

  for (const childBlock of describeBlock.children) {
    const childPassed = await runDescribeBlock(childBlock, indent + "  ");
    if (!childPassed) {
      allTestsPassedInBlock = false;
    }
  }

  return allTestsPassedInBlock;
}


export async function runTests() {
  totalTests = 0;
  passedTests = 0;

  await runDescribeBlock(rootDescribeBlock);

  console.log(colors.cyan("\n--- Test Summary ---"));
  console.log(colors.cyan(`Total Tests: ${totalTests}`));
  console.log(colors.green(`Passed: ${passedTests}`));
  console.log(colors.red(`Failed: ${totalTests - passedTests}`));
}
