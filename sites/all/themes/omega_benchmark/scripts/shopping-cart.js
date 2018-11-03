(function($) {
  $(document).ready( function() {
    //alert('Hello custom jQuery script inside jQuery wrapper');
	
    $('#benchmark-commerce-cart-block').tabSlideOut({
	tabHandle: '.tab-handle',                     //class of the element that will become your tab
	pathToTabImage: '/sites/all/themes/omega_benchmark/images/shopping-cart-tab.png', //path to the image for the tab //Optionally can be set using css
	imageHeight: '196px',                     //height of tab image           //Optionally can be set using css
	imageWidth: '40px',                       //width of tab image            //Optionally can be set using css
	tabLocation: 'right',                      //side of screen where tab lives, top, right, bottom, or left
	speed: 500,                               //speed of animation
	action: 'click',                          //options: 'click' or 'hover', action to trigger animation
	topPos: '300px',                          //position from the top/ use if tabLocation is left or right
	//leftPos: '20px',                          //position from left/ use if tabLocation is bottom or top
	fixedPosition: true                      //options: true makes it stick(fixed position) on scroll
    });
    
    $('.checkout-login-popup').click();
  });
})(jQuery);
