import { expect } from 'chai';
import { describe, it } from '../../test';

describe('ArrayBuffer', () => {
  it('should create and manipulate ArrayBuffer', () => {
    const buffer = new ArrayBuffer(8);
    const view = new Uint8Array(buffer);
    
    expect(buffer.byteLength).to.equal(8);
    expect(view.length).to.equal(8);

    view[0] = 255;
    expect(view[0]).to.equal(255);
  });

  it('should share memory between views', () => {
    const buffer = new ArrayBuffer(4);
    const view1 = new Uint8Array(buffer);
    const view2 = new Uint16Array(buffer);

    view1[0] = 0x12;
    view1[1] = 0x34;
    
    expect(view2[0]).to.equal(0x3412);
  });
});