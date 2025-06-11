

import { expect } from "chai";
import { describe, it, beforeEach, afterEach } from "../test";
import { filesystem } from "breeze";

const testDir = 'D:\\breeze-js\\tests\\js\\tests\\tests';

describe('filesystem', () => {
    beforeEach(async () => {
        await filesystem.mkdir(testDir, { recursive: true });
    });

    afterEach(async () => {
        await filesystem.rm(testDir, { recursive: true });
    });

    it('should write and read a file', async () => {
        const filePath = testDir + '/testfile.txt';
        const content = 'Hello, filesystem!';

        await filesystem.writeStringToFile(filePath, content);
        const readContent = await filesystem.readFileAsString(filePath);

        expect(readContent).to.equal(content, 'File content should match written content');
    });

    it('should create a directory', async () => {
        const dirPath = testDir + '/newdir';
        const existsBefore = filesystem.exists(dirPath);

        await filesystem.mkdir(dirPath, { recursive: true });
        const existsAfter = filesystem.exists(dirPath);

        expect(existsBefore).to.be.false;
        expect(existsAfter).to.be.true;
    });

    it('should list files in a directory', async () => {
        const dirPath = testDir + '/listdir';
        await filesystem.mkdir(dirPath, { recursive: true });
        const file1 = dirPath + '/file1.txt';
        const file2 = dirPath + '/file2.txt';

        await filesystem.writeStringToFile(file1, 'File 1');
        await filesystem.writeStringToFile(file2, 'File 2');

        const files = await filesystem.readdir(dirPath, { recursive: false, follow_symlinks: false });

        expect(files).to.include('file1.txt');
        expect(files).to.include('file2.txt');
    });

    it('should remove a file', async () => {
        const filePath = testDir + '/remove.txt';
        await filesystem.writeStringToFile(filePath, 'To be removed');

        const existsBefore = filesystem.exists(filePath);
        await filesystem.rm(filePath, { recursive: true });
        const existsAfter = filesystem.exists(filePath);

        expect(existsBefore).to.be.true;
        expect(existsAfter).to.be.false;
    });

    it('should remove a directory', async () => {
        const dirPath = testDir + '/removeDir';
        await filesystem.mkdir(dirPath, { recursive: true });
        const existsBefore = filesystem.exists(dirPath);

        await filesystem.rm(dirPath, { recursive: true });
        const existsAfter = filesystem.exists(dirPath);

        expect(existsBefore).to.be.true;
        expect(existsAfter).to.be.false;
    });

    it('should handle non-existent file removal gracefully', async () => {
        const filePath = testDir + '/nonexistent.txt';
        const existsBefore = filesystem.exists(filePath);

        await filesystem.rm(filePath, { recursive: true });
        const existsAfter = filesystem.exists(filePath);

        expect(existsBefore).to.be.false;
        expect(existsAfter).to.be.false;
    });

    it('should not crash', async () => {
        await Promise.allSettled([
            // @ts-ignore
            filesystem.rm(''),
            // @ts-ignore
            filesystem.mkdir('', {}),
            // @ts-ignore
            filesystem.readdir(''),
            filesystem.writeStringToFile('', ''),
            filesystem.readFileAsString('')
        ]);

        expect(true).to.be.true; // Just to ensure the test runs without crashing
    })
});