WorkerScript.onMessage = function(msg) {
    if (msg.action == 'loadImage') {
        msg.imageView.source = "image://local/" + msg.absolutePath;

        WorkerScript.sendMessage({ 'reply': msg.absolutePath + ' is loading' });
    }
}
