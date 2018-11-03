<?php $tag = $block->subject ? 'section' : 'div'; ?>
<<?php print $tag; ?><?php print $attributes; ?>>
  <div class="block-inner clearfix">
  	<div id="team-distribution-help" class="help-window">
   		<h4>Step 4: The Team Distribution List</h4>
        <p>Add your team's email addresses to the this field. When you are finished, click on the "Save Email Addresses to Team" button to finish adding team members.</p></div>
    <div id="team-distribution-section" class="team-ui-section team-section-outer">
      <div id="team-distribution-info-icon" class="team-section-info-icon"></div>
      <div id="team-distribution-inner-section" class="team-section-inner">
        <?php print render($title_prefix); ?>
        <?php if ($block->subject): ?>
          <h2<?php print $title_attributes; ?>><?php print $block->subject; ?></h2>
        <?php endif; ?>
        <?php print render($title_suffix); ?>
        
        <div<?php print $content_attributes; ?>>
          <?php print $content ?>
        </div>
      </div>
    </div>
  </div>
</<?php print $tag; ?>>
