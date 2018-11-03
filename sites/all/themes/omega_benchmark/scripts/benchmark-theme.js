(function($) {
  $(document).ready(function() {
	  
	$( "#logo" ).addClass( "svgswap" );
    
	
	$("a").filter(":contains('Click here to begin the survey.')").replaceWith( "<span>Click here to begin the survey.</span>" );
	
	$( "input[name*='diagnostic_item']" ).addClass( "TestClass" );
	
	
  //$( "input[name*='diagnostic_item']" ).iCheck({
//    checkboxClass: 'icheckbox_square-aero',
//    radioClass: 'iradio_square-aero',
//    increaseArea: '20%' // optional
//  });
	
    $('div.region-footer-first-inner section.triaxia-info').click(function() {
        window.location = "http://www.triaxiapartners.com";
    });
	
	/*window.onload = function (){
		
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
	
	};*/
	
	
	
	
var theBrowserType = null;

if ($.browser.msie) {
	        if ($.browser.version == '6.0') {
	            $('html').addClass('ie6');
	            console.log("Browser is ie6");
	            theBrowserType = "ie6";
	        } else if ($.browser.version == '7.0') {
	            $('html').addClass('ie7');
	            console.log("Browser is ie7");
	            theBrowserType = "ie7";
	        } else if ($.browser.version == '8.0') {
	            $('html').addClass('ie8');
	            console.log("Browser is ie8");
	            theBrowserType = "ie8";
	        } else if ($.browser.version == '9.0') {
	            $('html').addClass('ie9');
	            console.log("Browser is ie9");
	            theBrowserType = "ie9";
	        }
	    } else if ($.browser.webkit) {
	        $('html').addClass('webkit');
	        console.log("Browser is webkit");
	        theBrowserType = "webkit";
	    } else if ($.browser.mozilla) {
	        $('html').addClass('mozilla');
	        console.log("Browser is mozilla");
	        theBrowserType = "mozilla";
	    } else if ($.browser.opera) {
	        $('html').addClass('opera');
	        console.log("Browser is opera");
	        theBrowserType = "opera";
	    }

	    console.log("theBrowserType = " + theBrowserType);

	    if (theBrowserType == "ie7" || theBrowserType == "ie8") {
	        console.log("THIS IS IE 7 OR 8!!");
	        $('.svgswap').each(function () {
	            var srcLocation = $(this).attr('src').replace("svg", "png");
	            console.log("srcLocation = " + srcLocation);
	            $(this).attr("src", srcLocation);
	        });
	    };
    
  });
})(jQuery);
