<?php
/**
 * @file
 * Helper functions for the culturefeed agenda.
 **/

/**
 * Get the current active agenda object.
 * @param $path
 *   Path to load the active object of. Defaults to current path.
 */
function culturefeed_agenda_get_active_object($path = NULL) {

  $supported_types = array(
    'e' => 'culturefeed_agenda_event',
    'p' => 'culturefeed_agenda_production',
    'a' => 'culturefeed_agenda_actor',
  );

  if (arg(0, $path) == 'agenda' && isset($supported_types[arg(1, $path)])) {
    return menu_get_object($supported_types[arg(1, $path)], 3, $path);
  }

}

/**
 * Get all the links from a given item.
 * @var CultureFeed_Cdb_Item_Base $item
 */
function _culturefeed_agenda_get_links_from_item(CultureFeed_Cdb_Item_Base $item) {

  $detail = $item->getDetails()->getDetailByLanguage(culturefeed_search_get_preferred_language());

  $links = array();
  // General links.
  if ($detail) {

    foreach ($detail->getMedia() as $media) {
      if ($media->getMediaType() == CultureFeed_Cdb_Data_File::MEDIA_TYPE_WEBRESOURCE || $media->getMediaType() == CultureFeed_Cdb_Data_File::MEDIA_TYPE_WEBSITE) {
        $links[] = $media->getHLink();
      }
    }
  }

  // Contact info links.
  if (method_exists($item, 'getContactInfo') && $item->getContactInfo()) {
    $contact_links = _culturefeed_agenda_get_contact_links($item->getContactInfo());
    $links += $contact_links['main'];
    $links += $contact_links['other'];
  }

  // Location links.
  if (method_exists($item, 'getLocation') && $item->getLocation() && $item->getLocation()->getActor() && $item->getLocation()->getActor()->getContactInfo()) {
    $contact_links += _culturefeed_agenda_get_contact_links($item->getLocation()->getActor()->getContactInfo());
    $links += $contact_links['main'];
    $links += $contact_links['other'];
  }

  // Organiser links.
  if (method_exists($item, 'getOrganiser') && $item->getOrganiser() && $item->getOrganiser()->getActor() && $item->getOrganiser()->getActor()->getContactInfo()) {
    $contact_links = _culturefeed_agenda_get_contact_links($item->getOrganiser()->getActor()->getContactInfo());
    $links += $contact_links['main'];
    $links += $contact_links['other'];
  }

  // Performer links.
  if ($detail && method_exists($detail, 'getPerformers') && $detail->getPerformers()) {
    $performers = $detail->getPerformers();
    foreach ($performers as $performer) {
      if ($performer->getActor() && $performer->getActor()->getContactInfo()) {
        $contact_links = _culturefeed_agenda_get_contact_links($performer->getActor()->getContactInfo());
        $links += $contact_links['main'];
        $links += $contact_links['other'];
      }
    }
  }

  // Cleaning up links.
  foreach ($links as $i => $link) {
    $links[$i] = rtrim($link, '/');
  }

  $links = array_unique($links);

  return $links;

}

/**
 * Get the list of links from a given contact info. Seperate it in main and other.
 */
function _culturefeed_agenda_get_contact_links(CultureFeed_Cdb_Data_ContactInfo $info) {

  $links = array('main' => array(), 'other' => array());
  foreach ($info->getUrls() as $url) {
    if ($url->isMain()) {
      $links['main'][] = $url->getUrl();
    }
    else {
      $links['other'][] = $url->getUrl();
    }
  }

  return $links;

}

/**
 * Get the first culturefeed page link connected with the given item.
 */
function culturefeed_agenda_get_connected_page_id_of_item(CultureFeed_Cdb_Item_Base $item) {

  $detail = $item->getDetails()->getDetailByLanguage(culturefeed_search_get_preferred_language());

  // Reset organiser link. For CEP we need to check on pages for it.
  if ($detail && $detail->getMedia()) {

    $page_links = $detail->getMedia()->byMediaType(CultureFeed_Cdb_Data_File::MEDIA_TYPE_CULTUREFEED_PAGE);
    foreach ($page_links as $link) {

      if ($link->getRelationType() == CultureFeed_Cdb_Data_File::REL_TYPE_ORGANISER) {
        $link_parts = explode('/', $page_links->current()->getHLink());
        return $link_parts[count($link_parts) - 1];
      }
    }

  }

}

/**
 * Calculate the different row classes for a program by weekscheme.
 * @param CultureFeed_Cdb_Data_Calendar_Weekscheme $weekscheme
 * @param array $all_classes
 */
function culturefeed_agenda_calculate_program_classes_by_weekscheme(CultureFeed_Cdb_Data_Calendar_Weekscheme $weekscheme, $all_classes) {

  $classes = array();

  $current_day = date('l');
  $opening_info = $weekscheme->getDay($current_day);
  if ($opening_info->isOpen()) {

  }

}

/**
 * Get all the known locations for this item.
 * @param CultureFeed_Cdb_Item_Base $item
 */
function culturefeed_agenda_get_locations_of_item(CultureFeed_Cdb_Item_Base $item) {

  $locations = array();

  if ($item instanceof CultureFeed_Cdb_Item_Event) {
    $locations[] = culturefeed_agenda_get_location_of_event($item);
  }
  elseif ($item instanceof CultureFeed_Cdb_Item_Production) {
    $events = culturefeed_agenda_load_events_of_production($item, FALSE);
    if (!empty($events)) {
      foreach ($events as $event) {
        $locations[] = culturefeed_agenda_get_location_of_event($event->getEntity());
      }
    }
  }
  elseif ($item instanceof CultureFeed_Cdb_Item_Actor) {

    if ($item->getContactInfo()) {

      $contact_info = $item->getContactInfo();
      $addresses = $contact_info->getAddresses();

      if ($addresses) {

        $location = array();
        $location['title'] = $addresses[0]->getLabel();
        $physical_address = $addresses[0]->getPhysicalAddress();
        if ($physical_address) {
          $location['city'] = check_plain($physical_address->getCity());
          $location['street'] = check_plain($physical_address->getStreet() . ' ' . $physical_address->getHouseNumber());
          $location['zip'] = check_plain($physical_address->getZip());
        }
        $locations[] = $location;

      }

    }

  }

  return $locations;

}

/**
 * Get the location of an event.
 * @param CultureFeed_Cdb_Item_Event $event
 */
function culturefeed_agenda_get_location_of_event(CultureFeed_Cdb_Item_Event $event) {

  $location = array();
  if ($event->getLocation()) {

    // Always send a title key.
    $location['title'] = '';

    $location_info = $event->getLocation();

    if ($location_info->getLabel()) {
      $location['title'] = $location_info->getLabel();
    }
    if ($event->getCdbid()) {
      $location['cdbid_event'] = $event->getCdbid();
    }
    if ($event->getLocation()->getCdbid()) {
      $location['link'] = culturefeed_search_detail_l('actor', $event->getLocation()->getCdbid(), $location['title']);
    }
    if ($location_info->getActor()) {
      $actor_detail = $location_info->getActor()->getDetails()->getDetailByLanguage(culturefeed_search_get_preferred_language());
      if ($actor_detail) {
        $location['title'] = check_plain($actor_detail->getTitle());
      }
    }

    $physical_address = $location_info->getAddress()->getPhysicalAddress();
    if ($physical_address) {
      $location['city'] = $physical_address->getCity();
      $location['street'] = $physical_address->getStreet() . ' ' . $physical_address->getHouseNumber();
      $location['zip'] = $physical_address->getZip();
    }

  }

  return $location;

}

/**
 * Get the data for an item in the production program list.
 * @param Cultuurnet\Search\ActivityStatsExtendedEntity $item
 *   Extended entity of a CultureFeed_Cdb_Item_Event.
 */
function culturefeed_agenda_get_production_program_data_for_item(Cultuurnet\Search\ActivityStatsExtendedEntity $item) {

  $cdb_item = $item->getEntity();
  $detail = $cdb_item->getDetails()->getDetailByLanguage(culturefeed_search_get_preferred_language());

  // Only place items in list that have a detail for current language.
  if ($detail) {

    $location = culturefeed_agenda_get_location_of_event($cdb_item);

    // Construct base data.
    $data = array(
      'title' => $detail->getTitle(),
      'id' => $item->getId(),
      'url' => culturefeed_search_detail_url($item->getType(), $item->getId(), $detail->getTitle()),
      'when' => $detail->getCalendarSummary(),
      'city' => isset($location['city']) ? $location['city'] : '',
      'venue' => isset($location['title']) ? $location['title'] : '',
    );
    
    // Extend with an url to the actor.
    // This can be an organiser, or a location.
    $organiser = $cdb_item->getOrganiser();
    if ($organiser) {

      // Check if we can construct an actor url by label or actor reference..
      if ($organiser->getActor() && $organiser->getActor()->getDetails()->getDetailByLanguage(culturefeed_search_get_preferred_language())) {
        $actor = $organiser->getActor();
        $organiser_actor_details = $organiser->getActor()->getDetails()->getDetailByLanguage(culturefeed_search_get_preferred_language());
        $data['all_url'] = culturefeed_search_detail_url('actor', $actor->getCdbid(), $actor->getDetails()->getDetailByLanguage(culturefeed_search_get_preferred_language())->getTitle());
      }
      elseif ($organiser->getLabel() && $organiser->getCdbid()) {
        $data['all_url'] = culturefeed_search_detail_url('actor', $organiser->getCdbid(), $organiser->getLabel());
      }

    }

    // If we did not find an organiser. Check if we can link to the location.
    if (!isset($data['all_url']) && isset($location['link'])) {
      $data['all_url'] = culturefeed_search_detail_url('actor', $cdb_item->getLocation()->getCdbid(), $location['title']);
    }

  }

  $data['calendar'] = culturefeed_agenda_parse_calendar($cdb_item->getCalendar());

  return $data;
}

/**
 * Set the breadcrumb for an event / production.
 * @param \CultuurNet\Search\ActivityStatsExtendedEntity $item
 */
function culturefeed_agenda_set_activity_breadcrumb(\CultuurNet\Search\ActivityStatsExtendedEntity $item) {

  $active_trail = array();
  $facet = array();

  $active_trail[] = array(
    'title' => t('Home'),
    'href' => '<front>',
    'link_path' => '',
    'localized_options' => array(),
    'type' => 0,
  );

  // Show the event type in breadcrumb.
  $event_types = $item->getEntity()->getCategories()->getCategoriesByType(CultureFeed_Cdb_Data_Category::CATEGORY_TYPE_EVENT_TYPE);
  if ($event_types) {

    $facet['category_eventtype_id'] = array($event_types[0]->getId());

    // First event type.
    $active_trail[] = array(
      'title' => $event_types[0]->getName(),
      'href' => 'agenda/search',
      'link_path' => '',
      'localized_options' => array(
        'query' => array(
          'facet' => $facet,
        ),
      ),
      'type' => 0,
    );

  }

  // City
  $locations = culturefeed_agenda_get_locations_of_item($item->getEntity());
  if (!empty($locations) && isset($locations[0]['city'])) {
    $active_trail[] = array(
      'title' => $locations[0]['city'],
      'href' => 'agenda/search',
      'link_path' => '',
      'localized_options' => array(
        'query' => array(
          'location' => $locations[0]['zip'] . ' ' . $locations[0]['city'],
          'facet' => $facet,
        ),
      ),
      'type' => 0,
    );
  }


  $active_trail[] = array(
    'title' => $item->getTitle(culturefeed_search_get_preferred_language()),
    'href' => $_GET['q'],
    'link_path' => '',
    'localized_options' => array(),
    'type' => 0,
  );

  menu_set_active_trail($active_trail);

}

/**
 * Set the breadcrumb for an actor.
 * @param \CultuurNet\Search\ActivityStatsExtendedEntity $item
 */
function culturefeed_agenda_set_actor_breadcrumb(\CultuurNet\Search\ActivityStatsExtendedEntity $item) {

  $active_trail = array();

  $active_trail[] = array(
    'title' => t('Home'),
    'href' => '<front>',
    'link_path' => '',
    'localized_options' => array(),
    'type' => 0,
  );

  // Show the actor type in breadcrumb.
  $actor_types = $item->getEntity()->getCategories()->getCategoriesByType(CultureFeed_Cdb_Data_Category::CATEGORY_TYPE_ACTOR_TYPE);
  if ($actor_types) {

    // First event type.
    $active_trail[] = array(
      'title' => $actor_types[0]->getName(),
      'href' => 'agenda/search',
      'link_path' => '',
      'localized_options' => array(
        'query' => array(
          'facet' => array('category_actortype_id' => array($actor_types[0]->getId())),
        ),
      ),
      'type' => 0,
    );

  }

  // City
  $locations = culturefeed_agenda_get_locations_of_item($item->getEntity());
  if (!empty($locations) && isset($locations[0]['city'])) {
    $active_trail[] = array(
      'title' => $locations[0]['city'],
      'href' => 'agenda/search',
      'link_path' => '',
      'localized_options' => array(
        'query' => array(
          'location' => $locations[0]['zip'] . ' ' . $locations[0]['city'],
        ),
      ),
      'type' => 0,
    );
  }

  $active_trail[] = array(
    'title' => $item->getTitle(culturefeed_search_get_preferred_language()),
    'href' => $_GET['q'],
    'link_path' => '',
    'localized_options' => array(),
    'type' => 0,
  );

  menu_set_active_trail($active_trail);

}

/**
 * Get the render array to show the google map for road description.
 */
function culturefeed_agenda_get_map_render_array(\CultuurNet\Search\ActivityStatsExtendedEntity $item) {

  global $language;
  module_load_include('inc', 'culturefeed_agenda', 'theme/theme');
  $variables = array(
    'item' => $item,
    'title' => $item->getTitle($language->language),
    'link' => culturefeed_search_detail_l($item->getType(), $item->getId(), $item->getTitle($language->language)),
  );

  switch ($item->getType()) {

    case 'event':
      _culturefeed_agenda_preprocess_event($variables);
      break;

    case 'actor':
      _culturefeed_agenda_preprocess_actor($variables);
      break;
  }

  // Fetch the user info
  $cf_account = NULL;
  try {
    $cf_account = DrupalCultureFeed::getLoggedInUser();
  }
  catch (Exception $e) {
    watchdog_exception('culturefeed_agenda', $e);
  }

  if ($cf_account) {
    $variables['to_address'] = $cf_account->street . ', ' . $cf_account->zip . ' ' . $cf_account->city;
  }

  drupal_set_breadcrumb(array(l(t('Home'), NULL), $variables['link'], 'Stratenplan'));

  $query = array(
    'sensor' => 'true',
    'v' => '3.5',
    'region' => 'BE',
    'hl' => $language->language,
  );

  unset($variables['item']);

  $attached_js = array(
    array('data' => CULTUREFEED_GOOGLE_MAPS_JS_API, 'type' => 'external'),
    drupal_get_path('module', 'culturefeed_agenda') . '/js/map.js',
  );

  $attached_js[] = array(
    'type' => 'setting',
    'data' => array('culturefeed_map' => array('info' => $variables)),
  );

  $attached_js[] = array(
    'type' => 'inline',
    'data' => 'jQuery(document).ready(Drupal.CultureFeed.Agenda.initializeMap);',
  );

  return array(
    '#theme' => 'culturefeed_agenda_travel_map',
    '#attached' => array(
      'js' => $attached_js,
    ),
  );
}

/**
 * Load all related events for a production.
 * @param $production
 *   The production to load related events for.
 * @param $future_only
 *   FALSE to search also on past events.
 */
function culturefeed_agenda_load_events_of_production($production, $future_only = TRUE) {

  $relations = $production->getRelations();
  if (empty($relations)) {
    return;
  }

  // Currently we do an extra query to load the event details.
  // Sven checks if this could be added to the event loading.
  $cdb_ids = array();
  foreach ($relations as $relation) {
    $cdb_ids[] = 'cdbid:' . $relation->getCdbid();
  }

  $parameters = array();
  $parameters[] = new \CultuurNet\Search\Parameter\FilterQuery('type:event');
  $parameters[] = new \CultuurNet\Search\Parameter\Query(implode(' OR ', $cdb_ids));
  $parameters[] = new \CultuurNet\Search\Parameter\Group();

  if (!$future_only) {
    $parameters[] = new \CultuurNet\Search\Parameter\Parameter('past', 'true');
  }

  try {
    $result = culturefeed_get_search_service()->search($parameters);
    if ($result->getTotalCount() == 0) {
      return;
    }
  }
  catch (Exception $e) {
    watchdog_exception('culturefeed_agenda', $e);
    return;
  }

  return $result->getItems();
}


/**
 * Parse weekscheme from object to array
 */
function culturefeed_agenda_parse_weekscheme(CultureFeed_Cdb_Data_Calendar_Weekscheme $weekscheme) {

  $weekscheme_array = array();
  
  foreach ($weekscheme->getDays() as $day) {  
    $weekscheme_array[$day->getDayName()] = array(
      'type' => $day->getOpenType(),
    );
    if ($day->getOpenType() == 'open') {
      $i = 0;
      foreach($day->getOpeningTimes() as $openingtime) {
        $weekscheme_array[$day->getDayName()]['openingtimes'][$i]['from'] = $openingtime->getOpenFrom();
        if ($openingtime->getOpenTill()) {
          $weekscheme_array[$day->getDayName()]['openingtimes'][$i]['till'] = $openingtime->getOpenTill();
        }
        $i++;
      }
    }
  }
  return $weekscheme_array;
}

/**
 * Parse timestamps from object (CultureFeed_Cdb_Data_Calendar_TimestampList or CultureFeed_Cdb_Data_Calendar_Exceptions) to array
 */
function culturefeed_agenda_parse_timestamps($timestamps) {

  $timestamps_array = array();

  $timestamps_count = iterator_count($timestamps);
  $timestamps->rewind();
  $current_timestamp = 0;
  for ($i = 0; $i < $timestamps_count; $i++) {

    $timestamp = $timestamps->current();
    $date = $timestamp->getDate();
    $checkdate = new DateTime($date);
    $now = new DateTime(date('Y-m-d', time()));

    if ($checkdate >= $now) {
      $timestamps_array[$current_timestamp] = array(
        'day' => date('D', strtotime($date)),
        'date' => date('d/m', strtotime($date)),
      );
      if ($timestamp->getStartTime()) {
        $timestamps_array[$current_timestamp]['begintime'] = date('H:i', strtotime($timestamp->getStartTime()));
      }
      if ($timestamp->getEndTime()) {
        $timestamps_array[$current_timestamp]['endtime'] = date('H:i', strtotime($timestamp->getEndTime()));
      }
      if ($timestamp->getOpenType()) {
        $timestamps_array[$current_timestamp]['type'] = $timestamp->getOpenType();
      }
      $current_timestamp++;
    }
    $timestamps->next();

  }
  return $timestamps_array;
}

function culturefeed_agenda_group_timestamps(&$timestamps) {
  
  for ($i=0; $i<count($timestamps); $i++) {
    if (isset($timestamps[$i+1])) {
      $this_timestamp = $timestamps[$i];
      $next_timestamp = $timestamps[$i+1];

      if ($this_timestamp['date'] == $next_timestamp['date']) {
        if (!is_array($timestamps[$i]['begintime'])){  
          unset($timestamps[$i]['begintime']);
          $timestamps[$i]['begintime'][0] = $this_timestamp['begintime'];
          $timestamps[$i]['begintime'][1] = $next_timestamp['begintime'];
        }
        else {
          $timestamps[$i]['begintime'][] = $next_timestamp['begintime'];
        }
        sort($timestamps[$i]['begintime']);
        unset($timestamps[$i+1]);
        $timestamps = array_values($timestamps);
        $i--;
      }
    }
  }
  return $timestamps;
}

function culturefeed_agenda_parse_calendar($calendar) {

  $calendar_class = get_class($calendar);
  $calendar->rewind();

  if ($calendar_class == 'CultureFeed_Cdb_Data_Calendar_PeriodList') {
    
    $calendar_array = array(
      'type' => 'period',
      'period_start' => $calendar->current()->getDateFrom(),
      'period_end' => $calendar->current()->getDateTo(),
    );

    if ($calendar->current()->getWeekScheme()) {  
      $calendar_array['weekscheme'] = culturefeed_agenda_parse_weekscheme($calendar->current()->getWeekScheme());
    }
    if ($calendar->current()->getExceptions()) {
      $calendar_array['exceptions'] = culturefeed_agenda_parse_timestamps($calendar->current()->getExceptions());
    }
   
  }
  elseif ($calendar_class == 'CultureFeed_Cdb_Data_Calendar_Permanent') {
    $calendar_array = array(
      'type' => 'permanent',
    );
  
    if ($calendar->getWeekScheme()) {
      $calendar_array['weekscheme'] = culturefeed_agenda_parse_weekscheme($calendar->getWeekScheme());
    }
    if ($calendar->getExceptions()) {
      $calendar_array['exceptions'] = culturefeed_agenda_parse_timestamps($calendar->getExceptions());
    }
  
  }
  elseif ($calendar_class == 'CultureFeed_Cdb_Data_Calendar_TimestampList') {
    $calendar_array = array(
      'type' => 'timestamps',
    );
    $calendar_array['timestamps'] = culturefeed_agenda_parse_timestamps($calendar);
    $calendar_array['timestamps'] = culturefeed_agenda_group_timestamps($calendar_array['timestamps']);
  }
  return $calendar_array;
}