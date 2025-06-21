import { expect } from 'chai';
import { describe, it } from '../../test';

describe('Uint8Array', () => {
  it('should create and manipulate Uint8Array', () => {
    const arr = new Uint8Array([1, 2, 3]);
    expect(arr.length).to.equal(3);
    expect(arr[0]).to.equal(1);
    
    arr[1] = 5;
    expect(arr[1]).to.equal(5);
  });
});