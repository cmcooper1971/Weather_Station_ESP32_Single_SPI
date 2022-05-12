
// Get current readings when the page loads.

window.addEventListener('load', getReadings);

// Get current date and time function.

function updateDateTime() {

    var currentdate = new Date();
    var datetime = currentdate.getDate() + "/"
        + (currentdate.getMonth() + 1) + "/"
        + currentdate.getFullYear() + " at "
        + currentdate.getHours() + ":"
        + (currentdate.getMinutes() < 10 ? "0" : "") + currentdate.getMinutes() + ":"
        + (currentdate.getSeconds() < 10 ? "0" : "") + currentdate.getSeconds() ;

    document.getElementById("update-time").innerHTML = datetime;
    console.log(datetime);

} // Close function.

// Function to get current readings on the webpage when it loads for the first time. (dt.getMinutes() < 10 ? "0" : "") + dt.getMinutes();

function getReadings() {

    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {

            var myObj = JSON.parse(this.responseText);
            console.log(myObj);
            document.getElementById("sessionTimeArray0").innerHTML = myObj.sessionTimeArray0;
            document.getElementById("sessionTimeArray1").innerHTML = myObj.sessionTimeArray1;
            document.getElementById("sessionTimeArray2").innerHTML = myObj.sessionTimeArray2;
            document.getElementById("sessionTimeArray3").innerHTML = myObj.sessionTimeArray3;
            document.getElementById("sessionTimeArray4").innerHTML = myObj.sessionTimeArray4;
            document.getElementById("sessionTimeArray5").innerHTML = myObj.sessionTimeArray5;
            document.getElementById("sessionTimeArray6").innerHTML = myObj.sessionTimeArray6;
            document.getElementById("distanceTravelledArray0").innerHTML = myObj.distanceTravelledArray0;
            document.getElementById("distanceTravelledArray1").innerHTML = myObj.distanceTravelledArray1;
            document.getElementById("distanceTravelledArray2").innerHTML = myObj.distanceTravelledArray2;
            document.getElementById("distanceTravelledArray3").innerHTML = myObj.distanceTravelledArray3;
            document.getElementById("distanceTravelledArray4").innerHTML = myObj.distanceTravelledArray4;
            document.getElementById("distanceTravelledArray5").innerHTML = myObj.distanceTravelledArray5;
            document.getElementById("distanceTravelledArray6").innerHTML = myObj.distanceTravelledArray6;
            document.getElementById("tempMaxSpeed").innerHTML = myObj.tempMaxSpeed;
            document.getElementById("tempMaxSpeedDate").innerHTML = myObj.tempMaxSpeedDate;
            document.getElementById("tempSessionDistanceRecord").innerHTML = myObj.tempSessionDistanceRecord;
            document.getElementById("tempDistanceSDate").innerHTML = myObj.tempDistanceSDate;
            document.getElementById("tempSessionTimeRecord").innerHTML = myObj.tempSessionTimeRecord;
            document.getElementById("tempTimeSDate").innerHTML = myObj.tempTimeSDate;
            document.getElementById("tempDailyDistanceRecord").innerHTML = myObj.tempDailyDistanceRecord;
            document.getElementById("tempDistanceDDate").innerHTML = myObj.tempDistanceDDate;
            document.getElementById("tempDailyTimeRecord").innerHTML = myObj.tempDailyTimeRecord;
            document.getElementById("tempTimeDDate").innerHTML = myObj.tempTimeDDate;
            updateDateTime();
        }
    };

    xhr.open("GET", "/readings", true);
    xhr.send();

} // Close function.

// Create an Event Source to listen for events.

if (!!window.EventSource) {
    var source = new EventSource('/events');

    source.addEventListener('open', function (e) {
        console.log("Events Connected");
    }, false);

    source.addEventListener('error', function (e) {
        if (e.target.readyState != EventSource.OPEN) {
            console.log("Events Disconnected");
        }
    }, false);

    source.addEventListener('new_readings', function (e) {
        console.log("new_readings", e.data);
        var obj = JSON.parse(e.data);
        document.getElementById("sessionTimeArray0").innerHTML = Obj.sessionTimeArray0;
        document.getElementById("sessionTimeArray1").innerHTML = Obj.sessionTimeArray1;
        document.getElementById("sessionTimeArray2").innerHTML = Obj.sessionTimeArray2;
        document.getElementById("sessionTimeArray3").innerHTML = Obj.sessionTimeArray3;
        document.getElementById("sessionTimeArray4").innerHTML = Obj.sessionTimeArray4;
        document.getElementById("sessionTimeArray5").innerHTML = Obj.sessionTimeArray5;
        document.getElementById("sessionTimeArray6").innerHTML = Obj.sessionTimeArray6;
        document.getElementById("distanceTravelledArray0").innerHTML = Obj.distanceTravelledArray0;
        document.getElementById("distanceTravelledArray1").innerHTML = Obj.distanceTravelledArray1;
        document.getElementById("distanceTravelledArray2").innerHTML = Obj.distanceTravelledArray2;
        document.getElementById("distanceTravelledArray3").innerHTML = Obj.distanceTravelledArray3;
        document.getElementById("distanceTravelledArray4").innerHTML = Obj.distanceTravelledArray4;
        document.getElementById("distanceTravelledArray5").innerHTML = Obj.distanceTravelledArray5;
        document.getElementById("distanceTravelledArray6").innerHTML = Obj.distanceTravelledArray6;
        document.getElementById("tempMaxSpeed").innerHTML = Obj.tempMaxSpeed;
        document.getElementById("tempMaxSpeedDate").innerHTML = Obj.tempMaxSpeedDate;
        document.getElementById("tempSessionDistanceRecord").innerHTML = Obj.tempSessionDistanceRecord;
        document.getElementById("tempDistanceSDate").innerHTML = Obj.tempDistanceSDate;
        document.getElementById("tempSessionTimeRecord").innerHTML = Obj.tempSessionTimeRecord;
        document.getElementById("tempTimeSDate").innerHTML = Obj.tempTimeSDate;
        document.getElementById("tempDailyDistanceRecord").innerHTML = Obj.tempDailyDistanceRecord;
        document.getElementById("tempDistanceDDate").innerHTML = Obj.tempDistanceDDate;
        document.getElementById("tempDailyTimeRecord").innerHTML = Obj.tempDailyTimeRecord;
        document.getElementById("tempTimeDDate").innerHTML = Obj.tempTimeDDate;
        updateDateTime();
    }, false);

} // Close function.