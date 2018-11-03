

var triaxia = new Object();
triaxia.team_admin = new Object();


(function ($) {
  Drupal.behaviors.team_diagnostics = function(context) {
    $('#diagnostic-directions').hide();
    $( "#directions-header" ).click(function() {
      $('#diagnostic-directions').slideToggle(5);  
      return false;
    });
    
    //$('#email-team-container').hide();
    $( "#email-team-header" ).click(function() {
      $('#email-team-container').slideToggle(5);  
      return false;
    });
    
    //$('#add-team-container').hide();
    $( "#add-team-header" ).click(function() {
      $('#add-team-container').slideToggle(5);  
      return false;
    });
  }
})(jQuery);

(function ($) {
  $(document).ready(function() {
    
    Drupal.behaviors.team_diagnostics();
    
    $('textarea.textarea-hint').each(function(){
      // hides the input display text stored in the title on focus
      // and sets it on blur if the user hasn't changed it.
      //alert("hint fired");
      // show the display text
      $(this).each(function(i) {
          $(this).val($(this).attr('title'))
              .addClass('hint');
      });
  
      // hook up the blur & focus
      return $(this).focus(function() {
          if ($(this).val() == $(this).attr('title'))
              $(this).val('')
                  .removeClass('hint');
      }).blur(function() {
          if ($(this).val() == '')
              $(this).val($(this).attr('title'))
                  .addClass('hint');
      }); 
    });
    
    /* qTips for Team Admins */
    $('.team-section-info-icon').addClass('normal');
	
    // BEGIN Survey Finish Notification
	
	
		
   if ($('#dashboard-accordion')) {
            
			var accordianHeader = [];
           var accordianDivs = [];

            $(".team-sidebar-header-thingy").each(function() {
                if ($(this).parents("#dashboard-accordion")) {
                  accordianHeader.push($(this));
                  //console.log("Accordian Header Array Length is: " + accordianHeader.length);
				  	$(this).addClass("dashboard-header-"+accordianHeader.length);
                };					
            });
			$("div[id*='dashboard-item']").each(function () {
            $(this).each(function () {
                accordianDivs.push($(this));
                //console.log("Accordian Divs Array Length is: " + accordianDivs.length);
				 $(this).addClass("dashboard-report-area-"+accordianDivs.length);
				 //var myHTML = $(this).html();
				 //console.log(myHTML);
				 
				 if($(this).find('div.download-link').length != 0)
				 {
        			//console.log("Found");
					$(".dashboard-header-"+accordianDivs.length).addClass("download-ready");
					
     			}
            });
			});
			
			
			
			//accordianDivs.each(console.log("Accordian Divs Array wins"));
			/*console.log("Accordian Header Array Length is: " + accordianHeader.length);
			var ahL = accordianHeader.length;
			for (var i = 0; i < ahL; i++) {
				//$("#dashboard-item-"+ahL).addClass("HAPPY");
				console.log("acL is " + ahL);
			}*/

   };
		
		
		
		
			//if ($('#dashboard-accordion').children('.team-sidebar-header-thingy')) {
				//console.log("Found an H3");	
			//}
			
		
		
        /*$("div[id*='dashboard-item']").each(function () {
            $(this).each(function () {
                accordianDivs.push($(this));
                console.log("Accordian Divs Array Length is: " + accordianDivs.length);
            });
        });*/
		
			//for (var i=0,len=accordianDivs.length; i<len; i++) {
//				document.write(cars[i] + "<br>");
//			}
		
		
		
		//$( "a[title='Download the PDF']" ).addClass("Hello There");
			
			//console.log("Found One");

    
	$(document).ready(function() {
        // check cookie
        var visited = $.cookie("visited")

        if (visited == null) {
            /*$('.newsletter_layer').show();
            $.cookie('visited', 'yes'); 
            alert($.cookie("visited"));*/  
			//console.log("New Visitor detected. Get him!"); 
			      
        }

        // set cookie
        $.cookie('visited', 'yes', { expires: 1, path: '/' });
		
    });
	
	
	// END Survey Finish Notification
    
    $.extend($.ui.accordion.animations, {
      fastslide: function(options) {
                  $.ui.accordion.animations.slide(options, {duration: 10}); 
                 }
    });
    
    var accOpts = { animated: "fastslide"};
    $("#dashboard-accordion").accordion(accOpts);
    
  });

/*var tourdata = [
	   {
		  html: "Hello World"
	   },{
		  html: "Welcome to the Tour",
		  live: 5000,
		  delayIn: 500
	   }
	];
	var myTour = jTour(tourdata);
	myTour.start();*/

})(jQuery);


/*(function ($) {
  $(document).ready(function() {
	var tourdata = [
	   {
		  html: "Hello World"
	   },{
		  html: "Welcome to the Tour",
		  live: 5000,
		  delayIn: 500
	   }
	];
	var myTour = jTour(tourdata);
	myTour.start();
		

});

})(jQuery);*/



