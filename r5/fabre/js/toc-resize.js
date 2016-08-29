TOCContainerWidth="";
 
$(function() {


    resizeContent= function (){

        if ($(window).width() < 980)
        {
            $("#API-content").css("margin-left","");
            $("#TOC-container").css("width","");
            $("#fiware-logo-container").css("width","");
            $("#TOC-container").addClass("Responsive-TOC");
        }
        else
        {
           marginLeft=$('#TOC-container').width()-15;
           marginLeft += 15;
           $( "#API-content" ).css("margin-left",marginLeft);
           if ( $("#TOC-container").hasClass("Responsive-TOC") )
           {
                $('#TOC-container').css("width",TOCContainerWidth);
           }
           
           $("#TOC-container").removeClass("Responsive-TOC");
           TOCContainerWidth=$('#TOC-container').width();
            $("#fiware-logo-container").css("width", TOCContainerWidth);
        }
    };
    var timer = setInterval(resizeContent, 100);

});




