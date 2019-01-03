"use strict";

// The Cloud Functions for Firebase SDK to create Cloud Functions and setup triggers.
const functions = require('firebase-functions');

// My helper functions
const authenticate = require('./authenticate.js');

// The Firebase Admin SDK to access the Firebase Realtime Database.
const admin = require('firebase-admin');
admin.initializeApp(functions.config().firebase);

/**
 * Uses response object to send a response. I made this a separate function so that I can control its features later, particularly for testing purposes.
 * @param {*} response This is the response object received in the Firebase function.
 * @param {*} code This is the response code (e.g. 200).
 * @param {*} message This is the body of the response.
 */
function sendResponse(response, code, message) {
    return Promise.resolve().then(()=>response.status(code).send(message));
}

// // Create and Deploy Your First Cloud Functions
// // https://firebase.google.com/docs/functions/write-firebase-functions
//
exports.helloWorld = functions.https.onRequest((request, response) => {
    authenticate(admin, request).then((authResult) => {
        if (!authResult.result) {
            return sendResponse(response, 403, "Unauthorized, my dude!");
        }
        return sendResponse(response, 200, "Hello from Firebase!");
    }).catch(err=>{
        console.log(err);
        return sendResponse(response, 500, "Sorry, something broke.")
    });
});

function handleTrip() {
  const tripTime = new Date().toISOString();
  const payload = {
      notification: {
          title: 'Alarm has been tripped!',
          body: `${tripTime}`
      }
  };
  return admin.messaging().sendToTopic("trips", payload)
    .then((response) => {
        console.log('Notification sent:',response);
        return true;
    }).catch((error) => {
        console.log('Notification failed:', error);
    }).then(() => admin.firestore().collection('trips').add({time: tripTime}))
    
}

exports.tripAlarm = functions.https.onRequest((request, response) => {
    authenticate(admin, request).then((authResult) => {
        if (!authResult.result) {
            return sendResponse(response, 403, "Unauthorized, my dude!");
        }
        else {
            return handleTrip().then(writeResult => {
                return sendResponse(response, 200, "Alarm trip has been recorded.");
            })
        }
    }).catch(err=>{
        console.log(err);
        return sendResponse(response, 500, "Sorry, something broke.")
    });
});