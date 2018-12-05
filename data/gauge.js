google.charts.load('current', {'packages':['gauge']});
google.charts.setOnLoadCallback(drawChart);
 
function drawChart() {
function getth() {
 $.get("http://192.168.0.192/get?v=t", function(idata) {
  data.setValue(0, 1, idata.value);
  chart.draw(data, options);
 });
 $.get("http://192.168.0.192/get?v=h", function(idata) {
  data.setValue(1, 1, idata.value);
  chart.draw(data, options);
 });
}
 var data = google.visualization.arrayToDataTable([
  ['Label', 'Value'],
  ['T', 0],
  ['H', 0]
 ]);
 var options = {min: 0, max: 100, yellowFrom: 75, yellowTo: 100, minorTicks: 25};
 var chart = new google.visualization.Gauge(document.getElementById('chart_div'));
 getth();
 chart.draw(data, options);
 setInterval(getth, 30000);
}