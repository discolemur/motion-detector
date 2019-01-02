"use strict";

let assert = require('assert');
// let {google} = require('googleapis');
let requests = require('./requests.js');
let key = require('./secrets.json');

/**
 * This file tests deployed functions. I haven't yet figured out how to test locally...
 */

function getAccessToken() {
    return Promise.resolve().then(() => {
        return key.firebase_password;
        /*let SCOPE = "https://www.googleapis.com/auth/cloud-platform";
        let jwtClient = new google.auth.JWT(
          key.firebase_username,
          null,
          key.firebase_password,
          [SCOPE],
          null
        );
        jwtClient.authorize(function(err, tokens) {
          if (err) {
            reject(err);
            return;
          }
          resolve(tokens.access_token);
        });*/
    });
}
describe("Testing all functions", () => {
    describe("Test hello world", () => {
        it("should refuse request without authentication", () => {
            return requests.HelloWorld({token: null, user: ""}).then(response => {
                assert.equal(response.statusCode, 403);
                assert.equal(response.body, "Unauthorized, my dude!");
                return true;
            });
        });
        it("should refuse unauthorized user", () => {
            return requests.HelloWorld({token: "BIG CHUNGUS", user: "Bugs"}).then(response => {
                assert.equal(response.statusCode, 403);
                assert.equal(response.body, "Unauthorized, my dude!");
                return true;
            });
        });
        it("should accept a properly authorized user", () => {
            return getAccessToken()
                .then(authToken => requests.HelloWorld({token: authToken, user: key.firebase_username}))
                .then(response => {
                    assert.equal(response.statusCode, 200);
                    assert.equal(response.body, "Hello from Firebase!");
                    return true;
                });
        });
    });
});