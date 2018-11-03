<?php
/**
 * @file
 * benchmark_diagnostics_view.views_default.inc
 */

/**
 * Implements hook_views_default_views().
 */
function benchmark_diagnostics_view_views_default_views() {
  $export = array();

  $view = new view();
  $view->name = 'benchmark_diagnostics';
  $view->description = '';
  $view->tag = 'default';
  $view->base_table = 'node';
  $view->human_name = 'Benchmark Diagnostics';
  $view->core = 7;
  $view->api_version = '3.0';
  $view->disabled = FALSE; /* Edit this to true to make a default view disabled initially */

  /* Display: Master */
  $handler = $view->new_display('default', 'Master', 'default');
  $handler->display->display_options['title'] = 'Benchmark Diagnostics';
  $handler->display->display_options['css_class'] = 'diagnostic-list';
  $handler->display->display_options['use_more_always'] = FALSE;
  $handler->display->display_options['access']['type'] = 'perm';
  $handler->display->display_options['cache']['type'] = 'none';
  $handler->display->display_options['query']['type'] = 'views_query';
  $handler->display->display_options['exposed_form']['type'] = 'basic';
  $handler->display->display_options['pager']['type'] = 'full';
  $handler->display->display_options['pager']['options']['items_per_page'] = '10';
  $handler->display->display_options['style_plugin'] = 'list';
  $handler->display->display_options['style_options']['row_class'] = 'diagnostics-row';
  $handler->display->display_options['style_options']['default_row_class'] = FALSE;
  $handler->display->display_options['style_options']['class'] = 'thing-one';
  $handler->display->display_options['row_plugin'] = 'fields';
  /* Header: Global: Text area */
  $handler->display->display_options['header']['area']['id'] = 'area';
  $handler->display->display_options['header']['area']['table'] = 'views';
  $handler->display->display_options['header']['area']['field'] = 'area';
  /* Relationship: Content: Referenced product */
  $handler->display->display_options['relationships']['field_product_product_id']['id'] = 'field_product_product_id';
  $handler->display->display_options['relationships']['field_product_product_id']['table'] = 'field_data_field_product';
  $handler->display->display_options['relationships']['field_product_product_id']['field'] = 'field_product_product_id';
  $handler->display->display_options['relationships']['field_product_product_id']['required'] = TRUE;
  /* Field: Content: Title */
  $handler->display->display_options['fields']['title']['id'] = 'title';
  $handler->display->display_options['fields']['title']['table'] = 'node';
  $handler->display->display_options['fields']['title']['field'] = 'title';
  $handler->display->display_options['fields']['title']['label'] = '';
  $handler->display->display_options['fields']['title']['alter']['word_boundary'] = FALSE;
  $handler->display->display_options['fields']['title']['alter']['ellipsis'] = FALSE;
  /* Field: Content: Body */
  $handler->display->display_options['fields']['body']['id'] = 'body';
  $handler->display->display_options['fields']['body']['table'] = 'field_data_body';
  $handler->display->display_options['fields']['body']['field'] = 'body';
  $handler->display->display_options['fields']['body']['label'] = '';
  $handler->display->display_options['fields']['body']['element_label_colon'] = FALSE;
  /* Field: Content: Product Sample */
  $handler->display->display_options['fields']['field_product_sample']['id'] = 'field_product_sample';
  $handler->display->display_options['fields']['field_product_sample']['table'] = 'field_data_field_product_sample';
  $handler->display->display_options['fields']['field_product_sample']['field'] = 'field_product_sample';
  $handler->display->display_options['fields']['field_product_sample']['exclude'] = TRUE;
  $handler->display->display_options['fields']['field_product_sample']['click_sort_column'] = 'fid';
  $handler->display->display_options['fields']['field_product_sample']['type'] = 'file_url_plain';
  /* Field: Content: Product Image */
  $handler->display->display_options['fields']['field_product_image']['id'] = 'field_product_image';
  $handler->display->display_options['fields']['field_product_image']['table'] = 'field_data_field_product_image';
  $handler->display->display_options['fields']['field_product_image']['field'] = 'field_product_image';
  $handler->display->display_options['fields']['field_product_image']['label'] = '';
  $handler->display->display_options['fields']['field_product_image']['alter']['alter_text'] = TRUE;
  $handler->display->display_options['fields']['field_product_image']['alter']['text'] = '<div class="product-image"><a href="[field_product_sample]">[field_product_image]</a></div>';
  $handler->display->display_options['fields']['field_product_image']['element_label_colon'] = FALSE;
  $handler->display->display_options['fields']['field_product_image']['element_wrapper_class'] = 'benchmark-product-view-image';
  $handler->display->display_options['fields']['field_product_image']['click_sort_column'] = 'fid';
  $handler->display->display_options['fields']['field_product_image']['settings'] = array(
    'image_style' => '',
    'image_link' => '',
  );
  $handler->display->display_options['fields']['field_product_image']['group_column'] = 'fid';
  /* Field: Commerce Product: Add to Cart form */
  $handler->display->display_options['fields']['add_to_cart_form']['id'] = 'add_to_cart_form';
  $handler->display->display_options['fields']['add_to_cart_form']['table'] = 'commerce_product';
  $handler->display->display_options['fields']['add_to_cart_form']['field'] = 'add_to_cart_form';
  $handler->display->display_options['fields']['add_to_cart_form']['relationship'] = 'field_product_product_id';
  $handler->display->display_options['fields']['add_to_cart_form']['label'] = 'Purchase';
  $handler->display->display_options['fields']['add_to_cart_form']['element_label_colon'] = FALSE;
  $handler->display->display_options['fields']['add_to_cart_form']['show_quantity'] = 0;
  $handler->display->display_options['fields']['add_to_cart_form']['default_quantity'] = '1';
  $handler->display->display_options['fields']['add_to_cart_form']['combine'] = 1;
  $handler->display->display_options['fields']['add_to_cart_form']['display_path'] = 0;
  $handler->display->display_options['fields']['add_to_cart_form']['line_item_type'] = 0;
  /* Field: Global: Custom text */
  $handler->display->display_options['fields']['nothing']['id'] = 'nothing';
  $handler->display->display_options['fields']['nothing']['table'] = 'views';
  $handler->display->display_options['fields']['nothing']['field'] = 'nothing';
  $handler->display->display_options['fields']['nothing']['label'] = '';
  $handler->display->display_options['fields']['nothing']['alter']['text'] = '<hr /><a class="return-to-top" href="#top">Return to Top</a>';
  $handler->display->display_options['fields']['nothing']['element_type'] = '0';
  $handler->display->display_options['fields']['nothing']['element_label_colon'] = FALSE;
  $handler->display->display_options['fields']['nothing']['element_wrapper_type'] = 'div';
  $handler->display->display_options['fields']['nothing']['element_wrapper_class'] = 'divider';
  $handler->display->display_options['fields']['nothing']['element_default_classes'] = FALSE;
  /* Sort criterion: Content: Post date */
  $handler->display->display_options['sorts']['created']['id'] = 'created';
  $handler->display->display_options['sorts']['created']['table'] = 'node';
  $handler->display->display_options['sorts']['created']['field'] = 'created';
  $handler->display->display_options['sorts']['created']['expose']['label'] = 'Post date';
  /* Filter criterion: Content: Published */
  $handler->display->display_options['filters']['status']['id'] = 'status';
  $handler->display->display_options['filters']['status']['table'] = 'node';
  $handler->display->display_options['filters']['status']['field'] = 'status';
  $handler->display->display_options['filters']['status']['value'] = 1;
  $handler->display->display_options['filters']['status']['group'] = 0;
  $handler->display->display_options['filters']['status']['expose']['operator'] = FALSE;
  /* Filter criterion: Content: Type */
  $handler->display->display_options['filters']['type']['id'] = 'type';
  $handler->display->display_options['filters']['type']['table'] = 'node';
  $handler->display->display_options['filters']['type']['field'] = 'type';
  $handler->display->display_options['filters']['type']['value'] = array(
    'diagnostic' => 'diagnostic',
  );
  $handler->display->display_options['filters']['type']['group'] = '0';

  /* Display: Page */
  $handler = $view->new_display('page', 'Page', 'page');
  $handler->display->display_options['defaults']['hide_admin_links'] = FALSE;
  $handler->display->display_options['defaults']['header'] = FALSE;
  /* Header: Global: Text area */
  $handler->display->display_options['header']['area']['id'] = 'area';
  $handler->display->display_options['header']['area']['table'] = 'views';
  $handler->display->display_options['header']['area']['field'] = 'area';
  $handler->display->display_options['header']['area']['label'] = 'Diagnostics Intro Copy';
  $handler->display->display_options['header']['area']['content'] = '<div class="diagnostics-header">
<div class="diagnostics-intro"><h2>High performance teamwork rarely happens by accident.</h2>
<p class="diagnostics-intro-copy">It’s a lot of work, but the effort yields powerful results. The Team Benchmark family of diagnostics is designed to help you build teams that thrive and accomplish powerful results.</p>
<div class="pricing-table">
<h5 class="pricing-table-header">Diagnostic Pricing</h5>
<table class="diagnostic-prices">
  <tr>
    <th scope="col">Team  Size</th>
    <th scope="col">Cost Per Member</th>
  </tr>
  <tr class="odd">
    <td>1–5 Members</td>
    <td>$30</td>
  </tr>
  <tr class="even">
    <td>6–10 Members</td>
    <td>$26</td>
  </tr>
  <tr class="odd">
    <td>11–15 Members</td>
    <td>$22</td>
  </tr>
  <tr class="even">
    <td>16+ Members</td>
    <td>$18</td>
  </tr>
  <tr class="last">
    <td colspan="2">Please call us at 1-800-214-3917 for teams larger than 25 people</td>
  </tr>
</table>
</div>
</div>
<div class="video-row">
<div class="webcast-window"><h5>Watch the Team Benchmark<span class="superscript">®</span> Process Overview Video</h5>
<video width="256" height="155" controls preload="auto" poster="https://www.teambenchmark.com/sites/all/themes/omega_benchmark/images/webcastimage.jpg">
     <source src="/sites/default/files/video/purchase-short.mp4" type="video/mp4" />
     <source src="/sites/default/files/video/purchase-short.ogv" type="video/ogg" />
     <object width="480" height="300" type="application/x-shockwave-flash" data="http://fpdownload.adobe.com/strobe/FlashMediaPlayback.swf">
       <param name="movie" value="http://fpdownload.adobe.com/strobe/FlashMediaPlayback.swf"></param>
       <param name="flashvars" value="src=https%3A%2F%2Fwww.teambenchmark.com%2Fsites%2Fdefault%2Ffiles%2Fvideo%2Fpurchase-short.mp4&poster=https%3A%2F%2Fwww.teambenchmark.com%2Fsites%2Fall%2Fthemes%2Fomega_benchmark%2Fimages%2Fwebcastimage.jpg"></param>
       <param name="allowFullScreen" value="true"></param>
       <param name="allowscriptaccess" value="always"></param>
       <img src="https%3A%2F%2Fwww.teambenchmark.com%2Fsites%2Fall%2Fthemes%2Fomega_benchmark%2Fimages%2Fwebcastimage.jpg" alt="No Video Support" title="No Video Support">
     </object>
   </video>
</div>
<div class="webcast-copy"><p>View the video to the left for a brief explanation of the Team Benchmark purchasing and administration process. For more detailed information, please visit our <a href="https://www.teambenchmark.com/benchmark-process">Benchmark Process</a> page, or call us for questions at 1-800-214-3917. Our help desk is open between 8:30<span class="sub-cap"> am </span>and 5<span class="sub-cap"> pm EST,</span> Monday through Friday.</p></div>
</div>

<div class="diagnostic-thumbnails"><div class="development"><a href="#development"><img src="https://www.teambenchmark.com/sites/teambenchmark/files/Development-Cover.png"></a><h5>The Team Development Survey<span class="superscript">TM</span></h5></div><div class="performance"><a href="#performance"><img src="https://www.teambenchmark.com/sites/teambenchmark/files/Performance-Cover.png"></a><h5>The Team Performance Survey<span class="superscript">TM</span></h5></div><div class="communication"><a href="#communication"><img src="https://www.teambenchmark.com/sites/teambenchmark/files/Communication-Cover.png"></a><h5>The Team Communication Profile<span class="superscript">TM</span></h5></div><div class="meetings"><a href="#meetings"><img src="https://www.teambenchmark.com/sites/teambenchmark/files/Meetings-Cover.png"></a><h5>The Team Meetings Profile<span class="superscript">TM</span></h5></div><div class="conflict"><a href="#conflict"><img src="https://www.teambenchmark.com/sites/teambenchmark/files/Conflict-Cover.png"></a><h5>The Team Conflict Profile<span class="superscript">TM</span></h5></div></div></div>
</div>';
  $handler->display->display_options['header']['area']['format'] = 'full_html';
  $handler->display->display_options['path'] = 'benchmark-diagnostics';
  $export['benchmark_diagnostics'] = $view;

  return $export;
}
