const request = require('request-promise');
const config = require("../../.runtimeconfig.json");

let requests = {
    HelloWorld: (auth) => {
        let authorization = auth && auth.token ? `Bearer ${auth.token}` : "";
        return request({
            url: 'https://us-central1-discolemur-info.cloudfunctions.net/helloWorld',
            headers: {
            'X-Client-ID': auth.user,
            'Authorization': authorization
            }
        }).catch(err=>{
            console.log(err);
            return err.response;
        });
    }
}

module.exports = requests;