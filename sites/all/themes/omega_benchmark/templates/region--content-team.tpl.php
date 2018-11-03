<div<?php print $attributes; ?>>
  <div<?php print $content_attributes; ?>>
    <a id="main-content"></a>
    <?php print render($title_prefix); ?>
    <?php if ($title): ?>
    <?php if ($title_hidden): ?><div class="element-invisible"><?php endif; ?>
    <div id="team-name-section" class="team-ui-section team-section-outer">
      <div id="team-name-info-icon" class="team-section-info-icon"></div>
      <div id="team-name-inner-section" class="team-section-inner">
        <div class="field-label title-label">Team Name:&nbsp;</div>
        <h1 class="title" id="page-title"><?php print $title; ?></h1>
        <div class="team-title-description">(This team name will be visible on your report.)</div>
      </div>
    </div>
    <?php if ($title_hidden): ?></div><?php endif; ?>
    <?php endif; ?>
    <?php print render($title_suffix); ?>
    <?php if ($tabs && !empty($tabs['#primary'])): ?><div class="tabs clearfix"><?php print render($tabs); ?></div><?php endif; ?>
    <?php if ($action_links): ?><ul class="action-links"><?php print render($action_links); ?></ul><?php endif; ?>
    <?php print $content; ?>
    <?php if ($feed_icons): ?><div class="feed-icon clearfix"><?php print $feed_icons; ?></div><?php endif; ?>
  </div>
</div>