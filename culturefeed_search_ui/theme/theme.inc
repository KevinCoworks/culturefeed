<?php
/**
 * @file
 * Theme / preprocess functions for culturefeed search UI.
 */

/**
 * Preprocess function for the search page main section.
 *
 * @param array $vars
 */
function culturefeed_search_ui_preprocess_culturefeed_search_page(&$vars) {

  $vars['content'] = theme('culturefeed_search_list', array('result' => $vars['searchresult']));

  $vars['results_found'] = $vars['searchresult']->getTotalCount();

}

/**
 * Preprocess function for the search list.
 *
 * @param array $vars
 */
function culturefeed_search_ui_preprocess_culturefeed_search_list(&$vars) {

  if (!empty($vars['items'])) {
    $vars['result'] = $vars['items'];
  }
  $items = $vars['result']->getItems();

  $vars['noitems'] = $vars['result']->getTotalCount() <= 0 ? TRUE : FALSE;
  $vars['items'] = array();
  foreach ($items as $item) {
    $vars['items'][] = theme('culturefeed_' . $item->getType() . '_summary', array('item' => $item));
  }

}

/**
 * Theme the facet blocks.
 *
 * @param array $variables
 *   Containing the facet, items, sort, highest_level, sublevel visibility.
 *
 * @return string
 *   The markup for the facet block content.
 */
function theme_culturefeed_search_facet_with_depth_settings($variables) {
  // Limit the level and sublevels if needed.
  $facet_items = _culturefeed_search_ui_limit_facet_levels($variables['items'], $variables['highest_level'], $variables['show_subitems']);

  $facet_key = $variables['facet']->getKey();
  $path = empty($variables['path']) ? current_path() : $variables['path'];

  if (!isset($variables['query']) || !is_array($variables['query'])) {
    // Note that we need to remote the page parameter when adding/removing facet filters
    $query = drupal_get_query_parameters(NULL, array('q', 'page'));
  }
  else {
    $query = $variables['query'];
  }
  $query += array(
    'facet' => array()
  );

  $active_options = isset($query['facet'][$facet_key]) ? $query['facet'][$facet_key] : array();

  $facet_links = _culturefeed_search_ui_get_nested_facet_links($facet_items, $facet_key, $path, $query, $active_options, $variables['sort'], $variables['show_subitems']);
  $facet_links = $facet_links['facet_links'];

  // Allow others to alter the complete nested array with links.
  drupal_alter('culturefeed_search_ui_' . $facet_key . '_facet_links', $facet_links);

  if (is_array($facet_links)) {
    return theme('item_list', array(
      'items' => $facet_links,
      'type' => 'ul',
    ));
  }
}

/**
 * Helper function that recursively loops over the facet items and subitems.
 *
 * @param array $facet_items
 *   The facet item objects.
 * @param string $facet_key
 *   The key of the current facet type.
 * @param string $link_path
 *   The path to be used in the facet links.
 * @param array $query
 *   The query for the currently used search.
 * @param array $active_options
 *   The currently used options for the facet type.
 * @param string $sort_type
 *   The way the facet options should be sorted.
 * @param int $show_subitems
 *   Whether or not to show the subitems of the highest level.
 *
 * @return array
 *   facet_links: An array of themed links.
 *   has_active_subitem: A boolean indicating if there is an active option.
 */
function _culturefeed_search_ui_get_nested_facet_links($facet_items, $facet_key, $link_path, $query, $active_options, $sort_type, $show_subitems = 1) {
  $facet_links = array();
  $has_active_subitem = FALSE;

  foreach ($facet_items as $facet_item) {
    $sort = FALSE;
    // Construct unique key for sorting.
    if ($sort_type == CULTUREFEED_SEARCH_SORT_ALPHABETICALLY) {
      $facet_item_sort_values[] = $facet_item->getLabel();
      $sort = TRUE;
    }
    elseif ($sort_type == CULTUREFEED_SEARCH_SORT_TOTAL_RESULTS) {
      $facet_item_sort_values[] = $facet_item->getTotalResults();
      $sort = TRUE;
    }

    // If the list has an active subitem. Add class.
    $facet_item_active = in_array($facet_item->getValue(), $active_options);
    if ($facet_item_active) {
      $has_active_subitem = TRUE;
    }

    // Get the options for the facet link.
    $item = array(
      'data' => theme('culturefeed_search_facet_item', array(
        'facet_item' => $facet_item,
        'facet_key' => $facet_key,
        'query' => $query,
        'path' => $link_path,
        'active' => $facet_item_active,
      )),
    );

    // Get the subitems if there are any.
    if ($facet_item->hasSubItems() && ($show_subitems == 1)) {
      $subitems = _culturefeed_search_ui_get_nested_facet_links($facet_item->getSubItems(), $facet_key, $link_path, $query, $active_options, $sort_type);
      $item['children'] = $subitems['facet_links'];
      if ($subitems['has_active_subitem']) {
        $item['class'][] = 'active-parent';
        $has_active_subitem = TRUE;
      }
    }

    // Add the link item to the items array.
    $facet_links[] = $item;

    // Sort based on the sorting values.
    if ($sort) {
      array_multisort($facet_item_sort_values, $facet_links);
      if ($sort_type == CULTUREFEED_SEARCH_SORT_TOTAL_RESULTS) {
        $facet_links = array_reverse($facet_links);
        $facet_item_sort_values = array_reverse($facet_item_sort_values);
      }
    }
  }

  return array(
    'facet_links' => $facet_links,
    'has_active_subitem' => $has_active_subitem,
  );
}

/**
 * Helper function that limits the levels of items.
 *
 * @param array $facet_items
 *   The nested facet link items for theme_item_list().
 * @param int $highest_level
 *   A number indicating the highest level to be shown.
 *
 * @return array
 *   The altered facet link array, still ok for theme_item_list().
 */
function _culturefeed_search_ui_limit_facet_levels($facet_items, $highest_level = 1) {
  $current_level = 1;
  $new_items = array();

  // Get the required level.
  _culturefeed_search_ui_get_required_level($facet_items, $current_level, $highest_level, $new_items);

  return $new_items;
}

/**
 * Helper function that removes top levels from the facet links array as needed.
 *
 * @param array $facet_items
 *   The nested array with link items.
 * @param int $current_level
 *   The current level of recursiveness.
 * @param int $highest_level
 *   The highest level that is required.
 * @param array $new_items
 *   An array passed by reference to fill with the required levels.
 */
function _culturefeed_search_ui_get_required_level($facet_items, $current_level, $highest_level, &$new_items) {
  if ($current_level == $highest_level) {
    $new_items = array_merge($new_items, $facet_items);
  }
  else {
    // We have to go deeper.
    $we_have_items = FALSE;
    foreach ($facet_items as $link_item) {
      if ($link_item->hasSubItems()) {
        _culturefeed_search_ui_get_required_level($link_item->getSubItems(), ($current_level + 1), $highest_level, $new_items);
        $we_have_items = TRUE;
      }
    }
    // The highest level doesn't have any items, use the level above it.
    if (!$we_have_items) {
      $new_items = array_merge($new_items, $facet_items);
    }
  }
}

/**
 * Preprocess function for the search facet.
 * @see culturefeed-search-facet.tpl.php
 */
function culturefeed_search_ui_preprocess_culturefeed_search_facet(&$variables) {
  $facet = $variables['facet'];
  $path = empty($variables['path']) ? current_path() : $variables['path'];

  if (!isset($variables['query']) || !is_array($variables['query'])) {
    // Note that we need to remote the page parameter when adding/removing facet filters
    $query = drupal_get_query_parameters(NULL, array('q', 'page'));
  }
  else {
    $query = $variables['query'];
  }

  $query += array(
    'facet' => array()
  );

  $active_options = isset($query['facet'][$facet->getKey()]) ? $query['facet'][$facet->getKey()] : array();

  $items = array();
  $facets = $variables['items'];
  $sort_values = array();
  $sort = FALSE;

  // Add all the facet items to the build.
  foreach ($facets as $key => $facet_item) {
    $sub_item_output = array();
    $has_active_subitem = FALSE;

    if ($variables['sort'] == CULTUREFEED_SEARCH_SORT_ALPHABETICALLY) {
      $sort_values[] = $facet_item->getLabel();
      $sort = TRUE;
    }
    elseif ($variables['sort'] == CULTUREFEED_SEARCH_SORT_TOTAL_RESULTS) {
      $sort_values[] = $facet_item->getTotalResults();
      $sort = TRUE;
    }

    $sub_item_sort_values = array();
    // Add the subitems from this facet item.
    if ($facet_item->hasSubItems()) {

      $sub_items = $facet_item->getSubItems();

      // Create link for every sub item.
      foreach ($sub_items as $sub_item_key => $sub_item) {

        // Construct unique key for sorting.
        if ($variables['sort'] == CULTUREFEED_SEARCH_SORT_ALPHABETICALLY) {
          $sub_item_sort_values[] = $sub_item->getLabel();
        }
        elseif ($variables['sort'] == CULTUREFEED_SEARCH_SORT_TOTAL_RESULTS) {
          $sub_item_sort_values[] = $sub_item->getTotalResults();
        }

        // If the list has an active subitem. Add class.
        $sub_item_active = in_array($sub_item->getValue(), $active_options);
        if ($sub_item_active) {
          $has_active_subitem = TRUE;
        }

        $sub_item_output[] = theme('culturefeed_search_facet_item', array(
          'facet_item' => $sub_item,
          'facet_key' => $facet->getKey(),
          'query' => $query,
          'path' => $path,
          'active' => $sub_item_active,
        ));

      }

      // Sort based on the sorting values.
      if ($sort) {
        array_multisort($sub_item_sort_values, $sub_item_output);
        if ($variables['sort'] == CULTUREFEED_SEARCH_SORT_TOTAL_RESULTS) {
          $sub_item_output = array_reverse($sub_item_output);
          $sub_item_sort_values = array_reverse($sub_item_sort_values);
        }
      }
    }

    $item_active = in_array($facet_item->getValue(), $active_options);

    // Theme the item.
    $item_output = theme('culturefeed_search_facet_item', array(
      'facet_item' => $facet_item,
      'facet_key' => $facet->getKey(),
      'query' => $query,
      'path' => $path,
      'active' => $item_active,
    ));

    // Add it to the list.
    $items[] = array(
      'output' => $item_output,
      'sub_items' => $sub_item_output,
      'has_active_sub_item' => $has_active_subitem,
      'subitem_sort_values' => $sub_item_sort_values,
    );
  }

  // Sort based on the sorting values.
  if ($sort) {
    array_multisort($sort_values, $items);
    if ($variables['sort'] == CULTUREFEED_SEARCH_SORT_TOTAL_RESULTS) {
      $items = array_reverse($items);
    }
  }

  $variables['items'] = $items;
}

/**
 * Preprocess function for the search facet.
 * @see culturefeed-search-facet.tpl.php
 */
function culturefeed_search_ui_preprocess_culturefeed_search_facet_eventtype(&$variables) {

  culturefeed_search_ui_preprocess_culturefeed_search_facet($variables);

  $sort_values = array();
  foreach ($variables['items'] as $item) {
    if (!empty($item['sub_items'])) {
      foreach ($item['sub_items'] as $subitem_key => $sub_item) {
        $variables['event_types'][] = $sub_item;
        $sort_values[] = $item['subitem_sort_values'][$subitem_key];
      }
    }
  }

  // Sort based on the sorting values.
  if ($variables['sort'] == CULTUREFEED_SEARCH_SORT_ALPHABETICALLY || $variables['sort'] == CULTUREFEED_SEARCH_SORT_TOTAL_RESULTS) {
    array_multisort($sort_values, $variables['event_types']);
    if ($variables['sort'] == CULTUREFEED_SEARCH_SORT_TOTAL_RESULTS) {
      $variables['event_types'] = array_reverse($variables['event_types']);
    }
  }
}

/**
 * Preproces a search facet item.
 * @see culturefeed-search-facet-item.tpl.php
 */
function culturefeed_search_ui_preprocess_culturefeed_search_facet_item(&$variables) {
  $facet_key = $variables['facet_key'];
  $is_age_facet = $facet_key == 'agefrom';
  $facet_item = $variables['facet_item'];
  $query = $variables['query'];

  // Every filter will reset the current facet.
  // Remove this facet from URL, URL will function as removal URL.
  unset($query['facet'][$facet_key]);
  if ($is_age_facet) {
    unset($query['min_age']);
  }

  // When the facet is datetype, we don't want to keep a date range filter..
  if ($facet_key == 'datetype' && isset($query['date_range'])) {
    unset($query['date_range']);
  }

  // Agefrom facet is active when min_age is given.
  if ($is_age_facet && isset($_GET['min_age']) && $_GET['min_age'] == $facet_item->getValue()) {
    $variables['active'] = TRUE;
  }

  if ($facet_key == 'location' && isset($_GET['location'])) {
    // Location facet items are always active when location is in query string.
    $variables['active'] = TRUE;
    // When the facet is location, we don't need location/distance in the query.
    unset($query['location']);
    unset($query['distance']);
  }

  // Add this facet to the URL; the URL will function as additional filter URL.
  if (!$variables['active']) {
    // min_age is the key for agefrom.
    if ($is_age_facet) {
      $query['min_age'] = $facet_item->getValue();
    }
    else {
      $query['facet'][$facet_key][] = $facet_item->getValue();
    }
  }

  // Remove facet completely if nothing left.
  if (empty($query['facet'])) {
    unset($query['facet']);
  }

  $variables['url'] = url($variables['path'], array('query' => $query, 'absolute' => TRUE));
  $variables['label'] = $facet_item->getLabel();
  $variables['count'] = $facet_item->getTotalResults();
}

/**
 * Preprocess the sorting links.
 * @see theme_culturefeed_search_sort_links().
 */
function culturefeed_search_ui_preprocess_culturefeed_search_sort_links(&$variables) {

  $type = $variables['type'];

  $search_page = culturefeed_get_searchable_type($type);
  if (empty($search_page)) {
    return;
  }

  $sort_options = culturefeed_search_ui_get_sort_options_for_page($search_page);
  $current_sort = isset($_GET['sort']) ? $_GET['sort'] : $sort_options['default'];

  $query = drupal_get_query_parameters(NULL, array('q', 'page'));

  // Construct all the sort links.
  $links = array();
  foreach ($sort_options as $key => $option) {

    if ($key === 'default' || !$option['exposed']) {
      continue;
    }

    $query['sort'] = $option['value'];
    $attributes = array();
    if ($key === $current_sort || $option['value'] == $current_sort) {
      $attributes = array(
        'class' => array('active'),
      );
    }

    $links[] = array(
      'path' => current_path(),
      'text' => $option['label'],
      'options' => array(
        'html' => FALSE,
        'query' => $query,
        'attributes' => $attributes,
      )
    );

  }

  $variables['links'] = $links;

}

/**
 * Theme the pager summary for a search result.
 */
function theme_culturefeed_search_pager_summary($variables) {

  $result = $variables['result'];

  $end = $variables['start'] + $result->getCurrentCount();
  $args = array(
    '@range' => ($variables['start'] + 1) . '-' . $end,
  );

  $pager_summary = format_plural($result->getTotalCount(), '@range from @count result', '@range from @count results', $args);

  return $pager_summary;

}

/**
 * Theme the sorting links.
 */
function theme_culturefeed_search_sort_links($variables) {

  if (empty($variables['links'])) {
    return '';
  }

  $output = '<ul class="sort-links">';
  foreach ($variables['links'] as $link) {
    $output .= '<li>' . theme('link', $link) . '</li>';
  }
  $output .= '</ul>';

  return $output;


}

/**
 * Theme the admin form for creating custom filters.
 */
function theme_culturefeed_search_ui_admin_filter_form_options($variables) {

  $container = $variables['element'];

  $rows = array();
  foreach (element_children($container) as $key) {

    $row = array();
    foreach (element_children($container[$key]) as $child_key) {
      $container[$key][$child_key]['#attributes']['placeholder'] = $container[$key][$child_key]['#title'];
      unset($container[$key][$child_key]['#title']);
      $row[] = drupal_render($container[$key][$child_key]);
    }

    $rows[] = $row;
  }

  $table_variables = array(
    'header' => array(
      t('Exposed'),
      t('Human name'),
      t('Querystring for drupal url'),
      t('Filterquery for API')
    ),
    'rows' => $rows,
  );

  return theme('table', $table_variables);
}

/**
 * Theme an admin table for sort options of a search page.
 */
function theme_culturefeed_search_ui_admin_sort_order_options($variables) {

  $container = $variables['element'];

  $rows = array();
  foreach (element_children($container) as $key) {

    $row = array();
    foreach (element_children($container[$key]) as $child_key) {
      $container[$key][$child_key]['#attributes']['placeholder'] = $container[$key][$child_key]['#title'];
      unset($container[$key][$child_key]['#title']);
      $row[] = drupal_render($container[$key][$child_key]);
    }

    $rows[] = $row;
  }

  $table_variables = array(
    'header' => array(
      t('Default'),
      t('Exposed'),
      t('Label'),
      t('Value used in url'),
      t('Sortquery for api'),
    ),
    'rows' => $rows,
  );

  return theme('table', $table_variables);

}
