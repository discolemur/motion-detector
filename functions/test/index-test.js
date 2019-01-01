"use strict";

let assert = require('assert');
let requests = require('./requests.js');
let helloWorld = require('../index.js').helloWorld;

/**
 * This is a mock class for response that lets us ignore http communication for these tests.
 */
class Response {
    constructor() {
        this.message = null;
        this.code = null;
    }
    status(s) {
        this.code = s;
        return this;
    }
    send(m) {
        this.message = m;
        return Promise.resolve(this);
    }
}

describe("Testing all functions", ()=>{
    describe("Test hello world", ()=>{
        it("should refuse request without authentication", ()=>{
            let response = new Response();
            return helloWorld(requests.HelloWorld(null), response).then(response=>{
                assert.equal(response.code, 403);
                assert.equal(response.message, "Unauthorized, my dude!");
            });
        });
        it("should refuse unauthorized user", ()=>{
            let response = new Response();
            return helloWorld(requests.HelloWorld("BIG CHUNGUS"), response).then(response=>{
                assert.equal(response.code, 403);
                assert.equal(response.message, "Unauthorized, my dude!");
            });
        });
        it("should accept a properly authorized user", ()=>{
            let response = new Response();
            return helloWorld(requests.HelloWorld("TODO"), response).then(response=>{
                assert.equal(response.code, 200);
                assert.equal(response.message, "Hello from Firebase!");
            });
        });
    });
});
