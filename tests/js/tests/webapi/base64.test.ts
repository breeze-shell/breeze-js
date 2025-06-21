import { expect } from 'chai';
import { describe, it } from '../../test';

describe('Base64', () => {
  it('should encode to base64 with btoa', () => {
    const encoded = btoa('hello world');
    expect(encoded).to.equal('aGVsbG8gd29ybGQ=');
  });

  it('should decode from base64 with atob', () => {
    const decoded = atob('aGVsbG8gd29ybGQ=');
    expect(decoded).to.equal('hello world');
  });

  it('should handle Unicode characters', () => {
    const text = '你好世界';
    const encoded = btoa(text);
    const decoded = atob(encoded);
    expect(decoded).to.equal(text);
  });

//   it('should throw on invalid base64', () => {
//     expect(() => atob('invalid!')).to.throw();
//   });
});