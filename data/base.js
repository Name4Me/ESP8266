$(document).ready(function(e) {
 $(':button.button_pin').click(function(){
  tb($(this));
 });
 $.get("http://192.168.0.192/get?d=13", function(idata) {
  
  if (idata.pin13 == 0) {
   $("#pin13").addClass('button_on');
   $("#13").val("OFF").addClass('button_off');
  } else {
   $("#pin13").removeClass('button_on');
   $("#13").val("ON").removeClass('button_off');
  }
 
 });
 $.get("http://192.168.0.192/get?d=12", function(idata) {
  
  if (idata.pin12 == 0) {
   $("#pin12").addClass('button_on');
   $("#12").val("OFF").addClass('button_off');
  } else {
   $("#pin12").removeClass('button_on');
   $("#12").val("ON").removeClass('button_off');
  }
 
 });
});

function tb (elm){
 if (elm.hasClass('button_off')) {
  elm.removeClass('button_off');
  elm.val("ON");
  $("#pin"+elm.attr('id')).removeClass('button_on');
  $.post("http://192.168.0.192/set?d="+elm.attr('id')+"&v=1");
 } else {
  elm.addClass('button_off');
  elm.val("OFF");
  $("#pin"+elm.attr('id')).addClass('button_on');
  $.post("http://192.168.0.192/set?d="+elm.attr('id')+"&v=0");
 }        
}