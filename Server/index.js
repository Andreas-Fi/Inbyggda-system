//insert into db disabled @row 234

var app = require('express')();
var fs = require('fs');
var https = require('https');

//Reads the ssl certificate
var cert = fs.readFileSync(__dirname + '/sslcert/cert.cer');
var key = cert;
var credentials = { key: key, cert: cert, passphrase: "" };
var httpsServer = https.createServer(credentials, app);
var io = require('socket.io')(httpsServer);

var mqtt = require('mqtt');
var mqttClient = mqtt.connect('mqtt://APRaspberry');

//Sql connection setup
const sql = require('mssql');
const config = {
    password: 'EKmJcfVCPaX&8wfh',
    database: 'InlamningsUppgift',
    stream: false,
    options: {
        encrypt: false,
        readOnlyIntent: false
    },
    port: 1433,
    user: 'ReadUser',
    server: 'DATABASE',
    connectionLimit: 50,
    queueLimit: 10,
    pool: {
    },
}

let pool;
function createPool() {
    return new sql.ConnectionPool(config);
}

const poolRequest = createPool();
//Sql connection setup --End

//Sql connect
async function connect() {
    sql.on('error', (err) => {
        console.log('Mssql error event fired: ' + err);
    })
    const connection = await poolRequest.connect(config);

    console.log('Mssql connection established');
    connection.on('error', (err) => {
        console.log('Mssql connection error: ' + err);
        return process.exit(1);
    });
    pool = connection;
}

//Open database connection
connect().catch(err => { return process.exit(1); });
//Sql connect --End

///Inserts the collector data into the database
async function insertData(aggregatorID, deviceID, temperature, oxygen, powerState, collectedTime) {
    const request = pool.request();
    await new Promise((resolve) => {
        setTimeout(resolve, 10000);
    });

    request.input('aggregator', sql.VarChar(50), aggregatorID)
    request.input('device', sql.Int, deviceID);
    request.input('temperature', sql.Int, temperature);
    request.input('oxygen', sql.Int, oxygen);
    request.input('powerState', sql.Int, powerState);
    request.input('time', sql.DateTime, collectedTime);

    return request.query('insert into CollectorData (aggregatorID,deviceID,temperature,oxygen,powerState,collectedTime) values (@aggregatorID,@device,@temperature,@oxygen,@powerState,@time)').catch(err => {
        console.log('Insert Data error: ' + err);
    });
}

///Insert a new collectors nominal values into the database
async function insertNominal(aggregatorID, deviceID, nomTemperature, nomOxygen) {
    const request = pool.request();
    await new Promise((resolve) => {
        setTimeout(resolve, 10000);
    });
    //return request.query('select * from Nominals').catch(e => {
    request.input('nomTemperature', sql.Int, nomTemperature);
    request.input('nomOxygen', sql.Int, nomOxygen);
    request.input('device', sql.Int, deviceID);
    request.input('aggregatorID', sql.VarChar(50), aggregatorID);

    return request.query('insert into Nominals values (@aggregatorID,@device,@nomTemperature,@nomOxygen);').catch(err => {
        console.log('Insert Nominal error: ' + err);
    });
}

///Updates a collectors nominal values
async function updateNominals(aggregatorID, deviceID, nomTemperature, nomOxygen) {
    const request = pool.request();
    await new Promise((resolve) => {
        setTimeout(resolve, 10000);
    });

    //If all aggregators and all collectors
    if (aggregatorID == "all" && deviceID == 0 && typeof (nomTemperature) !== "undefined" && typeof (nomOxygen) !== "undefined") {
        request.input('nomTemperature', sql.Int, nomTemperature);
        request.input('nomOxygen', sql.Int, nomOxygen);
        return request.query('update Nominals set nomTemperature = @nomTemperature, nomOxygen = @nomOxygen').catch(e => {
            console.log('Update Nominal error: ' + e);
        });
    }
    //all aggregators and only nom temperature
    else if (aggregatorID == "all" && deviceID == 0 && typeof (nomTemperature) !== "undefined" && typeof (nomOxygen) === "undefined") {
        request.input('nomTemperature', sql.Int, nomTemperature);
        return request.query('update Nominals set nomTemperature = @nomTemperature').catch(e => {
            console.log('Update Nominal error: ' + e);
        });
    }
    //all aggregators and only nom oxygen
    else if (aggregatorID == "all" && deviceID == 0 && typeof (nomTemperature) === "undefined" && typeof (nomOxygen) !== "undefined") {
        request.input('nomOxygen', sql.Int, nomOxygen);
        return request.query('update Nominals set nomOxygen = @nomOxygen').catch(e => {
            console.log('Update Nominal error: ' + e);
        });
    }
    //If specific aggregator but all collectors
    else if (aggregatorID != "all" && deviceID == 0 && typeof (nomTemperature) !== "undefined" && typeof (nomOxygen) !== "undefined") {
        request.input('nomTemperature', sql.Int, nomTemperature);
        request.input('nomOxygen', sql.Int, nomOxygen);
        request.input('aggregatorID', sql.VarChar(50), aggregatorID);
        return request.query('update Nominals set nomTemperature = @nomTemperature, nomOxygen = @nomOxygen where aggregatorID = @aggregatorID').catch(e => {
            console.log('Update Nominal error: ' + e);
        });
    }
    //specific aggregator and only nom temperature
    else if (aggregatorID != "all" && deviceID == 0 && typeof (nomTemperature) !== "undefined" && typeof (nomOxygen) === "undefined") {
        request.input('nomTemperature', sql.Int, nomTemperature);
        request.input('aggregatorID', sql.VarChar(50), aggregatorID);
        return request.query('update Nominals set nomTemperature = @nomTemperature where aggregatorID = @aggregatorID').catch(e => {
            console.log('Update Nominal error: ' + e);
        });
    }
    //specific aggregator and only nom oxygen
    else if (aggregatorID != "all" && deviceID == 0 && typeof (nomTemperature) === "undefined" && typeof (nomOxygen) !== "undefined") {
        request.input('nomOxygen', sql.Int, nomOxygen);
        request.input('aggregatorID', sql.VarChar(50), aggregatorID);
        return request.query('update Nominals set nomOxygen = @nomOxygen where aggregatorID = @aggregatorID').catch(e => {
            console.log('Update Nominal error: ' + e);
        });
    }
    //If specific aggregator and specific collectors
    else if (aggregatorID != "all" && deviceID != 0 && typeof (nomTemperature) !== "undefined" && typeof (nomOxygen) !== "undefined") {
        request.input('nomTemperature', sql.Int, nomTemperature);
        request.input('nomOxygen', sql.Int, nomOxygen);
        request.input('device', sql.Int, deviceID);
        request.input('aggregatorID', sql.VarChar(50), aggregatorID);
        return request.query('update Nominals set nomTemperature = @nomTemperature, nomOxygen = @nomOxygen where deviceID = @device and aggregatorID = @aggregatorID').catch(e => {
            console.log('Update Nominal error: ' + e);
        });
    }
    //specific aggregator and collector and only nom temperature
    else if (aggregatorID != "all" && deviceID != 0 && typeof (nomTemperature) !== "undefined" && typeof (nomOxygen) === "undefined") {
        request.input('nomTemperature', sql.Int, nomTemperature);
        request.input('device', sql.Int, deviceID);
        request.input('aggregatorID', sql.VarChar(50), aggregatorID);
        return request.query('update Nominals set nomTemperature = @nomTemperature where deviceID = @device and aggregatorID = @aggregatorID').catch(e => {
            console.log('Update Nominal error: ' + e);
        });
    }
    //specific aggregator and collector and only nom oxygen
    else if (aggregatorID != "all" && deviceID != 0 && typeof (nomTemperature) === "undefined" && typeof (nomOxygen) !== "undefined") {
        request.input('nomOxygen', sql.Int, nomOxygen);
        request.input('device', sql.Int, deviceID);
        request.input('aggregatorID', sql.VarChar(50), aggregatorID);
        return request.query('update Nominals set nomOxygen = @nomOxygen where deviceID = @device and aggregatorID = @aggregatorID').catch(e => {
            console.log('Update Nominal error: ' + e);
        });
    }
    else {
        return;
    }
}

///Connect to the mqtt server
mqttClient.on('connect', function () {
    mqttClient.subscribe('novia/data', function (err) {
        if (!err) {
            //mqttClient.publish('presence', 'Hello mqtt');
            console.log('Mqtt connection established');
        }
        else {
            console.log('Mqtt on connect error: ' + err);
            return process.exit(1);
        }
    })
});

///Http get request for /
///Non-admin page
app.get('/', function (req, res) {
    res.sendFile(__dirname + '/Index.htm');
});

///Http get request for /admin
///Admin page
app.get('/admin', function (req, res) {
    res.sendFile(__dirname + '/adminIndex.htm');
});

///On client connection
io.on('connection', function (socket) {
    ///Message from admin client
    socket.on('message', function (msg) {
        mqttClient.publish('novia/data', msg);
    });

    ///Message from mqtt server
    mqttClient.on('message', function (topic, message) {
        socket.emit('message', message.toString());

        //Save to database
        let jsonObj = new Object();
        try {
            jsonObj = JSON.parse(message);
            //From aggregator
            if (jsonObj.hasOwnProperty('uid') && jsonObj.hasOwnProperty('temperature')
                && jsonObj.hasOwnProperty('oxygen') && jsonObj.hasOwnProperty('power')
                && jsonObj.hasOwnProperty('time') && jsonObj.hasOwnProperty('aggregatorID')) {

                //Save to DB data table
                // -- insertData(jsonObj.aggregatorID, Number(jsonObj.uid), jsonObj.temperature, jsonObj.oxygen, jsonObj.power, jsonObj.time);
            }
            //From admin
            else if (jsonObj.hasOwnProperty('uid') && jsonObj.hasOwnProperty('nomtemperature')
                && jsonObj.hasOwnProperty('nomoxygen') && jsonObj.hasOwnProperty('aggregatorID')) {
                //Update nominal temperature and nominal oxygen in nominal table
                updateNominals(jsonObj.aggregatorID, jsonObj.uid, jsonObj.nomtemperature, jsonObj.nomoxygen).then(res => {
                    if (res.hasOwnProperty('rowsAffected')) {
                        if (res.rowsAffected[0] == 0) {
                            insertNominal(jsonObj.aggregatorID, jsonObj.uid, jsonObj.nomtemperature, jsonObj.nomoxygen);
                        }
                    }
                });
            }
            else if (jsonObj.hasOwnProperty('uid') && jsonObj.hasOwnProperty('nomtemperature') && jsonObj.hasOwnProperty('aggregatorID')) {
                //Update nominal temperature in nominal table
                updateNominals(jsonObj.aggregatorID, jsonObj.uid, jsonObj.nomtemperature).then(res => {
                    if (res.hasOwnProperty('rowsAffected')) {
                        if (res.rowsAffected[0] == 0) {
                            insertNominal(jsonObj.aggregatorID, jsonObj.uid, jsonObj.nomtemperature, null);
                        }
                    }
                });
            }
            else if (jsonObj.hasOwnProperty('uid') && jsonObj.hasOwnProperty('nomoxygen') && jsonObj.hasOwnProperty('aggregatorID')) {
                //Update nominal oxygen in nominal table
                updateNominals(jsonObj.aggregatorID, jsonObj.uid, jsonObj.nomoxygen).then(res => {
                    if (res.hasOwnProperty('rowsAffected')) {
                        if (res.rowsAffected[0] == 0) {
                            insertNominal(jsonObj.aggregatorID, jsonObj.uid, null, jsonObj.nomoxygen);
                        }
                    }
                });
            }
        }
        catch (err) {
            console.log("Error in mqtt on message" + err);
        }
    });
});

//Starts listening on all IPv4 NICs on port 8443
httpsServer.listen(8443, "0.0.0.0", function (err) {
    if (err) {
        console.log("Https server error: " + err);
        return process.exit(1);
    }
    console.log('HTTPS: Listening on port 8443');
});