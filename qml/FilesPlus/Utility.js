.pragma library

function roundNumber(rnum, rlength) {
    // Arguments: number to round, number of decimal places

    var newnumber = Math.round(rnum*Math.pow(10,rlength))/Math.pow(10,rlength);
//    console.debug("roundNumber " + rnum + " -> " + newnumber);
    var value = parseFloat(newnumber);

    return value;
}

function formatFileSize(size, len) {
    if (!len) len = 0;

    var KB = 1024;
    var MB = 1048576;
    var GB = 1073741824;

    var fileSize = "";

    // Tested correctly on device, Qt Simulator will show incorrect decimal value. Example 3.45G shows 3G.
    if (size >= GB) {
        fileSize = roundNumber(size/GB, len) + " GB";
    } else if (size >= MB) {
        fileSize = roundNumber(size/MB, len) + " MB";
    } else if (size >= 1024) {
        fileSize = roundNumber(size/KB, len) + " KB";
    } else {
        fileSize = size + " B";
    }

    return fileSize;
}

function createJsonObj(jsonText) {
    var jsonObj = eval('(' + jsonText + ')');
    return jsonObj;
}
