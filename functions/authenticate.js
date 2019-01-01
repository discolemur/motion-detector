"use strict";

function AuthResult(user, result) {
    return {user: user, result: user ? result : false};
}

/**
 * 
 * @param {*} admin This is the firebase admin object. 
 * @param {*} request This is the http request that was received. It must have an authorization header that begins with "Bearer ".
 */
function authenticate(admin, req) {
    return Promise.resolve()
    .then(()=>{
        if (!req.headers.authorization || !req.headers.authorization.startsWith('Bearer ')) {
            console.log('Authorization header either not found or malformed!');
            return AuthResult(null, false);
        }
        let idToken =  req.headers.authorization.split('Bearer ')[1];
        return admin.auth().verifyIdToken(idToken).then((decodedIdToken) => {
            console.log('User identified from ID token: ', decodedIdToken);
            return AuthResult(decodedIdToken, true);
        });
    }).catch((error) => {
        console.error('Error while verifying Firebase ID token:', error);
        return AuthResult(null, false);
    });
}

module.exports = authenticate;