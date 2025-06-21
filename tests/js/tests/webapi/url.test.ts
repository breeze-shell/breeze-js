import { expect } from 'chai';
import { describe, it } from '../../test';

describe('URL', () => {
  it('should parse URL components', () => {
    const url = URL.parse('https://example.com:8080/path?query=1#hash', '');
    
    expect(url.protocol).to.equal('https:');
    expect(url.hostname).to.equal('example.com');
    expect(url.port).to.equal('8080');
    expect(url.pathname).to.equal('/path');
    expect(url.search).to.equal('?query=1');
    expect(url.hash).to.equal('#hash');
  });

  it('should handle URLSearchParams', () => {
    const params = new URLSearchParams('a=1&b=2');
    params.append('c', '3');
    
    expect(params.toString()).to.equal('a=1&b=2&c=3');
    expect(params.get('a')).to.equal('1');
    expect(params.getAll('b')).to.deep.equal(['2']);
  });

  it('should update search params via URL', () => {
    const url = URL.parse('https://example.com');
    url.searchParams.set('test', 'value');
    
    expect(url.toString()).to.equal('https://example.com/?test=value');
  });
});