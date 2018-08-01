<?php

// TODO Add dependency to file_entity (media) module because of:
//     * file_info_file_types()
//     * file_get_type()
//     * Field 'media_title'

// hook_block_info
// API:
//     https://api.drupal.org/api/drupal/modules%21block%21block.api.php/function/hook_block_info/7
// Example:
//     https://api.drupal.org/api/examples/block_example%21block_example.module/7
function eatlas_file_usage_block_info() {
	$blocks['eatlas_file_usage_list'] = array(
		'info' => t('eAtlas File usage'),
		'cache' => DRUPAL_NO_CACHE // Or maybe: DRUPAL_CACHE_PER_PAGE...
	);

	return $blocks;
}

/**
 * Implements hook_block_view().
 *
 * This hook generates the contents of the blocks themselves.
 */
function eatlas_file_usage_block_view($delta = '') {
	$block = array();

	switch ($delta) {
		case 'eatlas_file_usage_list':
			$block['content'] = _eatlas_file_usage_block_list_content();
			break;
	}

	return $block;
}

function _eatlas_file_usage_block_list_content() {
	$file = menu_get_object('file');
	$html_content = NULL;

	if ($file) {
		$usage = file_usage_list($file);
		$file_types_info = file_info_file_types();

		$nodes = array();
		$files = array();
		foreach($usage as $usage_module => $usage_value) {
			if ($usage_module !== 'media_file_delete') {
				if (isset($usage_value['node'])) {
					foreach($usage_value['node'] as $nid => $count) {
						$node = node_load($nid);
						if (node_access('view', $node) && !_eatlas_file_usage_block_is_prepress($node, $file)) {
							$type = node_type_get_name($node);
							if ($type) {
								if (!isset($nodes[$type])) {
									$nodes[$type] = array();
								}
								if (!isset($nodes[$type][$node->nid])) {
									$nodes[$type][$node->nid] = $node;
								}
							}
						}
					}
				}
				if (isset($usage_value['file'])) {
					foreach($usage_value['file'] as $fid => $count) {
						$file = file_load($fid);
						$can_view = user_access('view media'); // Depend on "Media" module
						if ($can_view) {
							$file_type = file_get_type($file);
							$type = $file_type;
							if (isset($file_types_info[$file_type]['label'])) {
								$type = $file_types_info[$file_type]['label'];
							}

							if (!isset($files[$type])) {
								$files[$type] = array();
							}
							if (!isset($files[$type][$file->fid])) {
								$files[$type][$file->fid] = $file;
							}
						}
					}
				}
			}
		}

		if (!empty($nodes) || !empty($files)) {
			// ksort => Sort an associative array by key
			ksort($nodes);
			ksort($files);

			$html_content = '<div class="file_usage_block">';
			foreach($nodes as $type => $nodeList) {
				if (!empty($nodeList)) {
					// uasort => Sort an associative array by value using a user-defined function
					uasort($nodeList, '_eatlas_file_usage_block_node_cmp');

					$html_content .= '<div class="title">'.$type.'</div>';
					$html_content .= '<ul>';
					foreach($nodeList as $nid => $node) {
						$html_content .= '<li>' . l($node->title, 'node/' . $node->nid) . '</li>';
					}
					$html_content .= '</ul>';
				}
			}

			foreach($files as $type => $fileList) {
				if (!empty($fileList)) {
					// uasort => Sort an associative array by value using a user-defined function
					uasort($fileList, '_eatlas_file_usage_block_file_cmp');

					$html_content .= '<div class="title">File: '.$type.'</div>';
					$html_content .= '<ul>';
					foreach($fileList as $fid => $file) {
						$html_content .= '<li>' . l(_eatlas_file_usage_block_get_file_title($file), 'media/' . $file->fid) . '</li>';
					}
					$html_content .= '</ul>';
				}
			}
			$html_content .= '</div>';
		}
	}

	return $html_content;
}

function _eatlas_file_usage_block_node_cmp($node_a, $node_b) {
	// Compare the 2 title, case insensitive
	return strcasecmp($node_a->title, $node_b->title);
}
function _eatlas_file_usage_block_file_cmp($file_a, $file_b) {
	// Compare the 2 title, case insensitive
	return strcasecmp(_eatlas_file_usage_block_get_file_title($file_a),
			_eatlas_file_usage_block_get_file_title($file_b));
}

function _eatlas_file_usage_block_get_file_title($file) {
	$titleField = field_get_items('file', $file, 'media_title');
	if ($titleField) {
		$titleValue = field_view_value('file', $file, 'media_title', $titleField[0]);
		if ($titleValue) {
			return render($titleValue);
		}
	}
	if (isset($file->filename)) {
		return $file->filename;
	}
	return 'Untitled';
}

/**
 * Returns FALSE if the node IS NOT in prepress mode
 *     OR the prepress field do not exists.
 * Returns TRUE if the node IS in prepress mode
 *     AND the user IS allowed to modified the file (image)
 *     (users allowed to modify the file needs to know if it's
 *     used somewhere)
 */
function _eatlas_file_usage_block_is_prepress($node, $file) {
	$prepress_field = field_get_items('node', $node, 'field_draft');
	// NOTE: Delta is always 0, it makes no sense to have multiple prepress checkboxes for a node.
	if ($prepress_field && isset($prepress_field[0]['value'])) {
		// The "!!" is used to change the value into a boolean variable
		$is_prepress = !!$prepress_field[0]['value'];
		if ($is_prepress) {
			// Check if the current user is allowed to modify the current file
			$can_edit_file = user_access('edit media'); // Depend on "Media" module

			if (!$can_edit_file) {
				// The node is prepress and the file can not be edited - the node reference needs to be hidden
				return TRUE;
			}
		}
	}
	// The node is not prepress OR the file can be edited - the node reference needs to be shown
	return FALSE;
}

?>
