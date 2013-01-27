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

function formatDistance(distance, len, unit) {
    if (!len) len = 0;
    if (!unit) unit = "m"; // Can be specified as m or ft.
    if (distance < 0) return "N/A";

    var km100 = 100000;
    var km = 1000;
    var ftTo100Mi = 528000;
    var ftToMi = 5280;

    var distanceText = "";

    if (unit == "m") {
        if (distance >= km100) {
            distanceText = roundNumber(distance/km, 0) + " km.";
        } else if (distance >= km) {
            distanceText = roundNumber(distance/km, len) + " km.";
        } else {
            distanceText = roundNumber(distance, len) + " m.";
        }
    } else if (unit == "ft") {
        if (distance >= ftTo100Mi) {
            distanceText = roundNumber(distance/ftToMi, 0) + " mi.";
        } else if (distance >= ftToMi) {
            distanceText = roundNumber(distance/ftToMi, len) + " mi.";
        } else {
            distanceText = roundNumber(distance, len) + " ft.";
        }
    }

    return distanceText;
}

function createJsonObj(jsonText) {
    var jsonObj = eval('(' + jsonText + ')');
    return jsonObj;
}

function createJsonText(jsonObj) {
    var jsonText = JSON.stringify(jsonObj);
    return jsonText;
}

function limit(v, min, max) {
    var newValue = v;
    newValue = Math.min(Math.max(v, min), max);
    return newValue;
}

function now() {
    return (new Date());
}

function nowText() {
    return Qt.formatDateTime(now(), "d MMM yyyy h:mm:ss ap");
}

function yesterday() {
    var n = new Date();
    n.setDate(n.getDate() - 1);

    return n;
}

function yesterdayUTC() {
    var n = new Date();
    n.setDate(n.getDate() - 1);

    return Date.UTC(n.getUTCFullYear(), n.getUTCMonth(), n.getUTCDate(), n.getUTCHours(), n.getUTCMinutes(), n.getUTCSeconds(), n.getUTCMilliseconds());
}

function lastWeek() {
    var n = new Date();
    n.setDate(n.getDate() - 7);

    return n;
}

function lastWeekUTC() {
    var n = new Date();
    n.setDate(n.getDate() - 7);

    return Date.UTC(n.getUTCFullYear(), n.getUTCMonth(), n.getUTCDate(), n.getUTCHours(), n.getUTCMinutes(), n.getUTCSeconds(), n.getUTCMilliseconds());
}

function getLocalDateTime(jsonDateString) {
    var d = new Date(jsonDateString);
    if (d == "Invalid Date") {
        d = parseJSONDate(jsonDateString);
    }

    var localeString = d.toLocaleString();
//    console.debug("getLocalDateTime jsonDateString " + jsonDateString + " -> " + d + " -> " + localeString);
    return localeString;
}

function getTime(jsonDateString) {
    var d = new Date(jsonDateString);
    if (d == "Invalid Date") {
        d = parseJSONDate(jsonDateString);
    }

    return d.getTime();
}

function compareTime(jsonDateStringA, jsonDateStringB) {
    var a = getTime(jsonDateStringA);
    var b = getTime(jsonDateStringB);
//    console.debug("Utility.compareTime " + jsonDateStringA + " " + jsonDateStringB + " " + (a-b));
    return a-b;
}

function parseDate(dateString) {
    var d = new Date(dateString);
//    console.debug("Utility.parseDate " + dateString + " " + d);
    if (d == "Invalid Date") {
        d = parseJSONDate(dateString);
    }

    return d;
}

function parseJSONDate(jsonDateString) {
//    console.debug("Utility.parseJSONDate jsonDateString " + jsonDateString);

    if (!jsonDateString) return jsonDateString;

    var re = /(\d{4})-(\d\d)-(\d\d)T(\d\d):(\d\d):(\d\d)(\.\d+)?(Z|([+-])(\d\d):(\d\d)|([+-])(\d\d\d\d))/;
    var d = jsonDateString.match(re);
    if (!d) return d; // Return d if it's null
//    console.debug("Utility.parseJSONDate d " + d + " d.length " + d.length);

    // "2010-12-07T11:00:00.000-09:00" parses to:
    //  ["2010-12-07T11:00:00.000-09:00", "2010", "12", "07", "11", "00", "00", ".000", "-09:00", "-", "09", "00"]
    // "2010-12-07T11:00:00.000Z" parses to:
    //  ["2010-12-07T11:00:00.000Z",      "2010", "12", "07", "11", "00", "00", ".000", "Z", undefined, undefined, undefined]

    var date = new Date();
    date.setUTCFullYear(d[1]);
    date.setUTCMonth(d[2]-1); // Month value 0-12. 0=January
    date.setUTCDate(d[3]);
    date.setUTCHours(d[4]);
    date.setUTCMinutes(d[5]);
    date.setUTCSeconds(d[6]);
//    console.debug("Utility.parseJSONDate initial date " + date + " from data " + d[1] + " " + (d[2]-1) + " " + d[3] + " " + d[4] + " " + d[5] + " " + d[6]);
    if (d[7]) {
//        console.debug("Utility.parseJSONDate setMsec date " + date + " msec " + d[7]);
        date.setUTCMilliseconds(d[7]);
    }
    if (d[8] != "Z") {
        // TODO incorrect minus timezone.
//        console.debug("Utility.parseJSONDate minusTimeZone is not implemented yet. date " + date + " timezone " + d[8]);
    }

//    console.debug("Utility.parseJSONDate date " + date);
    return date;
}

function replace(text, oldText, newText) {
    return text.replace(oldText, newText);
}

function distance(lat1, lon1, lat2, lon2) {
    var R = 6371; // km
    var d = Math.acos(Math.sin(lat1)*Math.sin(lat2) +
                      Math.cos(lat1)*Math.cos(lat2) *
                      Math.cos(lon2-lon1)) * R;
    return d;
}

function filterNumber(s) {
    if (!s) return -1;

//    console.debug("filterNumber s " + s);
    var n = s.replace(/[^\d\.\-]/g, '');
//    console.debug("filterNumber n " + n);
    return n;
}

function elideText(s, len) {
    var eText = s.substring(0,len) + "...";
    return eText;
}

function wrapText(s, len) {
    var text = s;
    var wText = "";
    while (text.length > len) {
        wText = wText + text.substring(0,len) + "\n";
        text = text.substring(len);
    }
    wText = wText + text;

    console.debug("wText " + wText);
    return wText;
}

function parseLink(text) {
    // Example RegEx (http|ftp|https)://[a-z0-9\-_]+(\.[a-z0-9\-_]+)+([a-z0-9\-\.,@\?^=%&;:/~\+#]*[a-z0-9\-@\?^=%&;/~\+#])?
    var caps = text.match("(http|https)(\:\/\/)([A-Za-z0-9\-\.,@\?^=%&;:/~\+#]+)");
    console.debug("parseLink caps " + caps);
    if (caps && caps.length > 0) {
        return caps[0];
    }

    return "";
}

function getIndexInArray(array, propertyName, propertyValue) {
    if (!array || !propertyName || !propertyValue) {
        return -1;
    }

    for (var i=0; i< array.length; i++) {
//        console.debug("getIndexInArray " + propertyName + " " + array[i][propertyName] + " " + propertyValue);
        if (array[i][propertyName] == propertyValue) {
            return i;
        }
    }

    return -1;
}

function ifUndefined(o, defaultValue) {
    return (!o) ? defaultValue : o;
}

function endWith(s1, s2) {
    return (s1.lastIndexOf(s2) == (s1.length-1));
}
