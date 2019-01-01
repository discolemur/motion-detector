let requests = {
    HelloWorld: (authID) => {
        return {
            type: "GET",
            headers: {
                authorization: authID
            }
        };
    }
}

module.exports = requests;