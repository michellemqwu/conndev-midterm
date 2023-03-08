const broker = 'wss://public.cloud.shiftr.io';

let client;
let prevTimestamp;
let prevFsr;

let options = {
  clean: true,
  connectTimeout: 10000,
  clientId: 'mqttJsClient-mmw451',
  username: 'public',
  password: 'public'
}

let topic = 'conndev/mmw451';

function setup() {
    localDiv = document.getElementById('local');
    readingStatus = document.getElementById('readingStatus');
    readingHistory = document.getElementById('readingHistory');
  
    localDiv.innerHTML = 'trying to connect';
    client = mqtt.connect(broker, options);
    client.on('connect', onConnect);
    client.on('close', onDisconnect);
    client.on('message', onMessage);
    client.on('error', onError);
}

function onConnect() {
    localDiv.innerHTML = 'connected to broker. Subscribing...'
    client.subscribe(topic, onSubscribe);
}

function onDisconnect() {
  localDiv.innerHTML = 'disconnected from broker.'
}

function onError(error) {
  localDiv.innerHTML = error;
}

function onSubscribe(response, error) {
  if (!error) {
    localDiv.innerHTML = 'Subscribed to broker.';
  } else {
    localDiv.innerHTML = error;
  }
}

function onMessage(topic, payload, packet) {
  let payloadString = payload.toString();
  let payloadJson = JSON.parse(payloadString);
  let fsr = payloadJson.fsr.toString();
  let timestamp = payloadJson.timestamp.toString();
  let stable = parseInt(payloadJson.stable);
  let content;
  let accelX = payloadJson.acce.acceX.toString();
  let accelY = payloadJson.acce.acceY.toString();
  let accelZ = payloadJson.acce.acceZ.toString();

  if (stable == 1) {
    content = '<h2>' + 'Bookmark is currently resting on fsr reading: '+ fsr + '</h2>';
    content += '<p>' + 'last time it was moved was at ' + prevTimestamp + ' with fsr reading at '+ prevFsr + '</p>';
  } else {
    console.log(stable);
    content = '<h2>' + 'Bookmark is being moved at '+ timestamp + '</h2>';
    content += '<p>' + 'fsr reading is ' + fsr + '</p>';
    prevTimestamp = timestamp;
    prevFsr = fsr;
  }
  readingStatus.innerHTML = content;
}

document.addEventListener('DOMContentLoaded', setup);