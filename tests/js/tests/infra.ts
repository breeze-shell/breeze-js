

import { expect } from "chai";
import { describe, it, runTests } from "../test";
import { infra } from "breeze";

describe("infra", () => {
  describe("sleep", () => {
    it("should resolve after the specified time", async () => {
      const start = Date.now();
      await infra.sleep(100);
      const duration = Date.now() - start;
      expect(duration).to.be.at.least(100);
    });

    it("should not block the event loop", async () => {
      let executed = false;
      infra.setTimeout(() => {
        executed = true;
      }, 50);
      await infra.sleep(100);
      expect(executed).to.be.true;
    });

    it("should handle zero duration", async () => {
      const start = Date.now();
      await infra.sleep(0);
      const duration = Date.now() - start;
      expect(duration).to.be.lessThan(50);
    });

    it("should handle negative duration", async () => {
      const start = Date.now();
      await infra.sleep(-100);
      const duration = Date.now() - start;
      expect(duration).to.be.lessThan(50);
    });
  });

  describe("sleepSync", () => {
    it("should block the event loop for the specified time", () => {
      const start = Date.now();
      infra.sleepSync(100);
      const duration = Date.now() - start;
      expect(duration).to.be.at.least(100);
    });

    it("should handle zero duration without blocking", () => {
      const start = Date.now();
      infra.sleepSync(0);
      const duration = Date.now() - start;
      expect(duration).to.be.lessThan(50);
    });

    it("should handle negative duration without blocking", () => {
      const start = Date.now();
      infra.sleepSync(-100);
      const duration = Date.now() - start;
      expect(duration).to.be.lessThan(50);
    });
  });

  describe("setTimeout", () => {
    it("should execute callback after specified time", async () => {
      let executed = false;
      await new Promise<void>((resolve) => {
        infra.setTimeout(() => {
          executed = true;
          resolve();
        }, 100);
      });
      expect(executed).to.be.true;
    });

    it("should return a timeout ID", () => {
      const id = infra.setTimeout(() => { }, 100);
      expect(id).to.be.a("number");
    });

    it("should not execute callback if cleared", async () => {
      let executed = false;
      const id = infra.setTimeout(() => {
        executed = true;
      }, 100);
      infra.clearTimeout(id);
      await infra.sleep(150);
      expect(executed).to.be.false;
    });
  });

  describe("setInterval", () => {
    it("should execute callback repeatedly", async () => {
      let count = 0;
      const id = infra.setInterval(() => {
        count++;
      }, 100);
      await infra.sleep(450);
      expect(count).to.be.at.least(3);
    });

    it("should return an interval ID", () => {
      const id = infra.setInterval(() => { }, 100);
      expect(id).to.be.a("number");
    });

    it("should not execute callback if cleared", async () => {
      let executed = false;
      const id = infra.setInterval(() => {
        executed = true;
      }, 100);
      infra.clearInterval(id);
      await infra.sleep(150);
      expect(executed).to.be.false;
    })

    it("should handle zero interval", async () => {
      let count = 0;
      const id = infra.setInterval(() => {
        count++;
        if (count === 3) {
          infra.clearInterval(id);
        }
      }, 0);
      await infra.sleep(200);
      expect(count).to.be.at.least(3);
    });
  });

  describe("atob", () => {
    it("should decode base64 strings", () => {
      const input = "SGVsbG8sIFdvcmxkIQ=="; // "Hello, World!"
      const output = infra.atob(input);
      expect(output).to.equal("Hello, World!");
    });

    it("should handle empty strings", () => {
      const output = infra.atob("");
      expect(output).to.equal("");
    });
  });

  describe("btoa", () => {
    it("should encode strings to base64", () => {
      const input = "Hello, World!";
      const output = infra.btoa(input);
      expect(output).to.equal("SGVsbG8sIFdvcmxkIQ==");
    });

    it("should handle empty strings", () => {
      const output = infra.btoa("");
      expect(output).to.equal("");
    });
  });
});
