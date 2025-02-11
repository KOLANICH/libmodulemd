/*
 * This file is part of libmodulemd
 * Copyright (C) 2018 Red Hat, Inc.
 *
 * Fedora-License-Identifier: MIT
 * SPDX-2.0-License-Identifier: MIT
 * SPDX-3.0-License-Identifier: MIT
 *
 * This program is free software.
 * For more information on the license, see COPYING.
 * For more information on free software, see <https://www.gnu.org/philosophy/free-sw.en.html>.
 */

#include <glib.h>
#include <glib/gstdio.h>
#include <locale.h>
#include <signal.h>

#include "modulemd-defaults.h"
#include "modulemd-module.h"
#include "modulemd-module-index.h"
#include "modulemd-module-index-merger.h"
#include "modulemd-module-stream.h"
#include "modulemd-module-stream-v2.h"
#include "private/glib-extensions.h"
#include "private/modulemd-defaults-v1-private.h"
#include "private/modulemd-module-private.h"
#include "private/modulemd-util.h"
#include "private/modulemd-yaml.h"
#include "private/test-utils.h"

typedef struct _ModuleFixture
{
} ModuleFixture;

extern int modulemd_test_signal;

static void
module_test_construct (ModuleFixture *fixture, gconstpointer user_data)
{
  g_autoptr (ModulemdModule) m = NULL;
  g_autoptr (GPtrArray) list = NULL;
  g_autoptr (GError) error = NULL;

  /* Test that the new() function works */
  m = modulemd_module_new ("testmodule");
  g_assert_nonnull (m);
  g_assert_true (MODULEMD_IS_MODULE (m));
  g_assert_true (modulemd_module_validate (m, &error));
  g_assert_no_error (error);
  g_assert_cmpstr (modulemd_module_get_module_name (m), ==, "testmodule");
  g_assert_null (modulemd_module_get_defaults (m));
  list = modulemd_module_get_streams_by_stream_name_as_list (m, "teststream");
  g_assert_cmpint (list->len, ==, 0);
  g_clear_pointer (&list, g_ptr_array_unref);
  g_assert_null (
    modulemd_module_get_stream_by_NSVCA (m, "test", 42, "test", NULL, &error));
  g_assert_error (error, MODULEMD_ERROR, MODULEMD_ERROR_NO_MATCHES);
  g_clear_error (&error);
  g_clear_object (&m);

  /* Test that object instantiation works wiht a name */
  m = g_object_new (MODULEMD_TYPE_MODULE, "module-name", "testmodule", NULL);
  g_assert_true (MODULEMD_IS_MODULE (m));
  g_assert_cmpstr (modulemd_module_get_module_name (m), ==, "testmodule");
  g_clear_object (&m);

  /* Test that we abort with a NULL name to new() */
  modulemd_test_signal = 0;
  signal (SIGTRAP, modulemd_test_signal_handler);
  m = modulemd_module_new (NULL);
  g_assert_cmpint (modulemd_test_signal, ==, SIGTRAP);
  g_clear_object (&m);

  /* Test that we abort if we instantiate without a name */
  modulemd_test_signal = 0;
  signal (SIGTRAP, modulemd_test_signal_handler);
  m = g_object_new (MODULEMD_TYPE_MODULE, NULL);
  g_assert_cmpint (modulemd_test_signal, ==, SIGTRAP);
  g_clear_object (&m);

  /* test that we abort if we instantiate with a NULL name */
  modulemd_test_signal = 0;
  signal (SIGTRAP, modulemd_test_signal_handler);
  m = g_object_new (MODULEMD_TYPE_MODULE, "module-name", NULL, NULL);
  g_assert_cmpint (modulemd_test_signal, ==, SIGTRAP);
  g_clear_object (&m);
}


static void
module_test_defaults (ModuleFixture *fixture, gconstpointer user_data)
{
  g_autoptr (ModulemdModule) m = NULL;
  g_autoptr (ModulemdDefaultsV1) d = NULL;
  g_autoptr (GError) nested_error = NULL;
  ModulemdDefaults *d_got = NULL;

  m = modulemd_module_new ("testmodule");

  /* Verify that setting defaults that don't match this module name fails and
   * returns an error
   */
  d = modulemd_defaults_v1_new ("test");
  g_assert_nonnull (d);

  g_assert_cmpint (
    modulemd_module_set_defaults (
      m, MODULEMD_DEFAULTS (d), MD_DEFAULTS_VERSION_UNSET, &nested_error),
    ==,
    MD_DEFAULTS_VERSION_ERROR);
  g_assert_nonnull (nested_error);
  g_clear_object (&d);
  g_clear_pointer (&nested_error, g_error_free);

  d = modulemd_defaults_v1_new ("testmodule");
  g_assert_nonnull (d);
  g_assert_cmpint (
    modulemd_module_set_defaults (
      m, MODULEMD_DEFAULTS (d), MD_DEFAULTS_VERSION_UNSET, NULL),
    ==,
    MD_DEFAULTS_VERSION_ONE);

  d_got = modulemd_module_get_defaults (m);
  g_assert_nonnull (d_got);
  g_assert_cmpstr (
    modulemd_defaults_get_module_name (d_got), ==, "testmodule");

  g_assert_cmpint (
    modulemd_module_set_defaults (m, NULL, MD_DEFAULTS_VERSION_UNSET, NULL),
    ==,
    MD_DEFAULTS_VERSION_UNSET);
  g_assert_null (modulemd_module_get_defaults (m));
}


static void
module_test_streams (ModuleFixture *fixture, gconstpointer user_data)
{
  g_autoptr (ModulemdModule) m = modulemd_module_new ("testmodule");
  g_autoptr (ModulemdTranslation) t = NULL;
  g_autoptr (ModulemdTranslationEntry) te = NULL;
  g_autoptr (GError) error = NULL;
  ModulemdModuleStream *stream = NULL;
  GPtrArray *list = NULL;

  /* Create a translation pre-adding streams */
  te = modulemd_translation_entry_new ("nl_NL");
  modulemd_translation_entry_set_summary (te, "Een test omschrijving");
  t = modulemd_translation_new (1, "testmodule", "stream1", 42);
  modulemd_translation_set_translation_entry (t, te);
  g_clear_pointer (&te, g_object_unref);
  modulemd_module_add_translation (m, t);
  g_clear_pointer (&t, g_object_unref);

  /* Create and add some streams to cross */
  stream = modulemd_module_stream_new (2, "testmodule", "stream1");
  modulemd_module_stream_set_version (stream, 1);
  modulemd_module_stream_set_context (stream, "context1");
  modulemd_module_stream_v2_set_summary (MODULEMD_MODULE_STREAM_V2 (stream),
                                         "Stream 1");
  g_assert_cmpint (modulemd_module_add_stream (
                     m, stream, MD_MODULESTREAM_VERSION_UNSET, NULL),
                   ==,
                   MD_MODULESTREAM_VERSION_TWO);
  g_clear_object (&stream);
  stream = modulemd_module_stream_new (2, "testmodule", "stream1");
  modulemd_module_stream_set_version (stream, 3);
  modulemd_module_stream_set_context (stream, "context2");
  modulemd_module_stream_v2_set_summary (MODULEMD_MODULE_STREAM_V2 (stream),
                                         "Stream 1");
  g_assert_cmpint (modulemd_module_add_stream (
                     m, stream, MD_MODULESTREAM_VERSION_UNSET, NULL),
                   ==,
                   MD_MODULESTREAM_VERSION_TWO);
  g_clear_object (&stream);
  stream = modulemd_module_stream_new (2, "testmodule", "stream1");
  modulemd_module_stream_set_version (stream, 1);
  modulemd_module_stream_set_context (stream, "context2");
  modulemd_module_stream_v2_set_summary (MODULEMD_MODULE_STREAM_V2 (stream),
                                         "Stream 1");
  g_assert_cmpint (modulemd_module_add_stream (
                     m, stream, MD_MODULESTREAM_VERSION_UNSET, NULL),
                   ==,
                   MD_MODULESTREAM_VERSION_TWO);
  g_clear_object (&stream);
  stream = modulemd_module_stream_new (2, "testmodule", "stream2");
  modulemd_module_stream_set_version (stream, 42);
  modulemd_module_stream_set_context (stream, "context42");
  modulemd_module_stream_v2_set_summary (MODULEMD_MODULE_STREAM_V2 (stream),
                                         "Stream 2");
  g_assert_cmpint (modulemd_module_add_stream (
                     m, stream, MD_MODULESTREAM_VERSION_UNSET, NULL),
                   ==,
                   MD_MODULESTREAM_VERSION_TWO);
  g_clear_object (&stream);

  /* Create a translation post-adding streams */
  te = modulemd_translation_entry_new ("en_GB");
  modulemd_translation_entry_set_summary (te, "A test summary");
  t = modulemd_translation_new (1, "testmodule", "stream2", 42);
  modulemd_translation_set_translation_entry (t, te);
  g_clear_pointer (&te, g_object_unref);
  modulemd_module_add_translation (m, t);
  g_clear_pointer (&t, g_object_unref);

  /* Verify that we get all streams */
  list = modulemd_module_get_all_streams (m);
  g_assert_cmpint (list->len, ==, 4);
  list = NULL; /* get_all_streams is transfer none */

  /* Test by_stream_name */
  list =
    modulemd_module_get_streams_by_stream_name_as_list (m, "nosuchstream");
  g_assert_nonnull (list);
  g_assert_cmpint (list->len, ==, 0);
  g_clear_pointer (&list, g_ptr_array_unref);

  list = modulemd_module_get_streams_by_stream_name_as_list (m, "stream2");
  g_assert_nonnull (list);
  g_assert_cmpint (list->len, ==, 1);
  stream = (ModulemdModuleStream *)g_ptr_array_index (list, 0);
  g_assert_cmpstr (
    modulemd_module_stream_get_context (stream), ==, "context42");
  g_clear_pointer (&list, g_ptr_array_unref);

  /* Verify that ordering in the by_stream_name is right */
  list = modulemd_module_get_streams_by_stream_name_as_list (m, "stream1");
  g_assert_nonnull (list);
  g_assert_cmpint (list->len, ==, 3);
  stream = (ModulemdModuleStream *)g_ptr_array_index (list, 0);
  g_assert_nonnull (stream);
  g_assert_cmpint (modulemd_module_stream_get_version (stream), ==, 3);
  stream = (ModulemdModuleStream *)g_ptr_array_index (list, 1);
  g_assert_nonnull (stream);
  g_assert_cmpint (modulemd_module_stream_get_version (stream), ==, 1);
  stream = (ModulemdModuleStream *)g_ptr_array_index (list, 2);
  g_assert_nonnull (stream);
  g_assert_cmpint (modulemd_module_stream_get_version (stream), ==, 1);
  g_clear_pointer (&list, g_ptr_array_unref);

  /* Get streams by NSVC */
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  stream = modulemd_module_get_stream_by_NSVC (m, "nosuch", 3, "nosuchctx");
  g_assert_null (stream);
  g_clear_error (&error);

  stream = modulemd_module_get_stream_by_NSVC (m, "stream1", 1, "context1");
  g_assert_nonnull (stream);
  G_GNUC_END_IGNORE_DEPRECATIONS

  /* Get streams by NSVCA */
  stream = modulemd_module_get_stream_by_NSVCA (
    m, "nosuch", 3, "nosuchctx", NULL, &error);
  g_assert_null (stream);
  g_assert_error (error, MODULEMD_ERROR, MODULEMD_ERROR_NO_MATCHES);
  g_clear_error (&error);

  stream =
    modulemd_module_get_stream_by_NSVCA (m, "stream1", 1, NULL, NULL, &error);
  g_assert_null (stream);
  g_assert_error (error, MODULEMD_ERROR, MODULEMD_ERROR_TOO_MANY_MATCHES);
  g_clear_error (&error);

  stream = modulemd_module_get_stream_by_NSVCA (
    m, "stream1", 1, "context1", NULL, &error);
  g_assert_nonnull (stream);
  g_assert_no_error (error);

  g_assert_cmpstr (
    modulemd_module_stream_get_stream_name (stream), ==, "stream1");
  g_assert_cmpint (modulemd_module_stream_get_version (stream), ==, 1);
  g_assert_cmpstr (
    modulemd_module_stream_get_context (stream), ==, "context1");
  g_assert_cmpstr (modulemd_module_stream_v2_get_summary (
                     MODULEMD_MODULE_STREAM_V2 (stream), NULL),
                   ==,
                   "Stream 1");
  g_assert_cmpstr (modulemd_module_stream_v2_get_summary (
                     MODULEMD_MODULE_STREAM_V2 (stream), "nl_NL"),
                   ==,
                   "Een test omschrijving");

  stream = modulemd_module_get_stream_by_NSVCA (
    m, "stream1", 1, "context2", NULL, &error);
  g_assert_nonnull (stream);
  g_assert_no_error (error);

  g_assert_cmpstr (
    modulemd_module_stream_get_stream_name (stream), ==, "stream1");
  g_assert_cmpint (modulemd_module_stream_get_version (stream), ==, 1);
  g_assert_cmpstr (
    modulemd_module_stream_get_context (stream), ==, "context2");

  stream = modulemd_module_get_stream_by_NSVCA (
    m, "stream1", 3, "context1", NULL, &error);
  g_assert_null (stream);
  g_assert_error (error, MODULEMD_ERROR, MODULEMD_ERROR_NO_MATCHES);
  g_clear_error (&error);

  stream = modulemd_module_get_stream_by_NSVCA (
    m, "stream1", 3, "context2", NULL, &error);
  g_assert_nonnull (stream);
  g_assert_no_error (error);

  g_assert_cmpstr (
    modulemd_module_stream_get_stream_name (stream), ==, "stream1");
  g_assert_cmpint (modulemd_module_stream_get_version (stream), ==, 3);
  g_assert_cmpstr (
    modulemd_module_stream_get_context (stream), ==, "context2");

  stream = modulemd_module_get_stream_by_NSVCA (
    m, "stream2", 42, "context42", NULL, &error);
  g_assert_nonnull (stream);
  g_assert_no_error (error);

  g_assert_cmpstr (
    modulemd_module_stream_get_stream_name (stream), ==, "stream2");
  g_assert_cmpint (modulemd_module_stream_get_version (stream), ==, 42);
  g_assert_cmpstr (
    modulemd_module_stream_get_context (stream), ==, "context42");
  g_assert_cmpstr (modulemd_module_stream_v2_get_summary (
                     MODULEMD_MODULE_STREAM_V2 (stream), NULL),
                   ==,
                   "Stream 2");
  g_assert_cmpstr (modulemd_module_stream_v2_get_summary (
                     MODULEMD_MODULE_STREAM_V2 (stream), "en_GB"),
                   ==,
                   "A test summary");
}


static void
module_test_get_stream_names (ModuleFixture *fixture, gconstpointer user_data)
{
  g_autoptr (ModulemdModule) m = NULL;
  g_autoptr (ModulemdModuleStream) stream = NULL;
  g_auto (GStrv) list = NULL;

  /*Test module with no streams*/
  m = modulemd_module_new ("testmodule");
  list = modulemd_module_get_stream_names_as_strv (m);
  g_assert_nonnull (list);
  g_assert_cmpint (g_strv_length (list), ==, 0);
  g_clear_pointer (&list, g_strfreev);
  g_clear_object (&m);

  /*Test module with all same stream names*/
  m = modulemd_module_new ("testmodule");
  stream = modulemd_module_stream_new (2, "testmodule", "stream1");
  modulemd_module_add_stream (m, stream, MD_MODULESTREAM_VERSION_UNSET, NULL);
  g_clear_object (&stream);
  stream = modulemd_module_stream_new (2, "testmodule", "stream1");
  modulemd_module_add_stream (m, stream, MD_MODULESTREAM_VERSION_UNSET, NULL);
  g_clear_object (&stream);

  list = modulemd_module_get_stream_names_as_strv (m);
  g_assert_nonnull (list);
  g_assert_cmpint (g_strv_length (list), ==, 1);

  g_clear_pointer (&list, g_strfreev);
  g_clear_object (&m);

  /*Test module with all different stream names*/
  m = modulemd_module_new ("testmodule");
  stream = modulemd_module_stream_new (2, "testmodule", "stream1");
  modulemd_module_add_stream (m, stream, MD_MODULESTREAM_VERSION_UNSET, NULL);
  g_clear_object (&stream);
  stream = modulemd_module_stream_new (2, "testmodule", "stream2");
  modulemd_module_add_stream (m, stream, MD_MODULESTREAM_VERSION_UNSET, NULL);
  g_clear_object (&stream);
  stream = modulemd_module_stream_new (2, "testmodule", "stream3");
  modulemd_module_add_stream (m, stream, MD_MODULESTREAM_VERSION_UNSET, NULL);
  g_clear_object (&stream);

  list = modulemd_module_get_stream_names_as_strv (m);
  g_assert_nonnull (list);
  g_assert_cmpint (g_strv_length (list), ==, 3);

  g_assert_cmpstr (list[0], ==, "stream1");
  g_assert_cmpstr (list[1], ==, "stream2");
  g_assert_cmpstr (list[2], ==, "stream3");

  g_clear_pointer (&list, g_strfreev);
  g_clear_object (&m);

  /*Test module with some same/different stream names*/
  m = modulemd_module_new ("testmodule");
  stream = modulemd_module_stream_new (2, "testmodule", "stream1");
  modulemd_module_add_stream (m, stream, MD_MODULESTREAM_VERSION_UNSET, NULL);
  g_clear_object (&stream);
  stream = modulemd_module_stream_new (2, "testmodule", "stream1");
  modulemd_module_add_stream (m, stream, MD_MODULESTREAM_VERSION_UNSET, NULL);
  g_clear_object (&stream);
  stream = modulemd_module_stream_new (2, "testmodule", "stream2");
  modulemd_module_add_stream (m, stream, MD_MODULESTREAM_VERSION_UNSET, NULL);
  g_clear_object (&stream);

  list = modulemd_module_get_stream_names_as_strv (m);
  g_assert_nonnull (list);
  g_assert_cmpint (g_strv_length (list), ==, 2);

  g_assert_cmpstr (list[0], ==, "stream1");
  g_assert_cmpstr (list[1], ==, "stream2");

  g_clear_pointer (&list, g_strfreev);
  g_clear_object (&m);
}


static void
modulemd_test_remove_streams (void)
{
  g_autoptr (ModulemdModuleIndex) f29 = NULL;
  g_autoptr (ModulemdModuleIndex) f29_updates = NULL;
  g_autoptr (ModulemdModuleIndex) index = NULL;
  g_autoptr (ModulemdModuleIndexMerger) merger = NULL;
  ModulemdModule *nodejs_module = NULL;
  g_autoptr (GError) error = NULL;
  g_autoptr (GPtrArray) failures = NULL;
  g_autofree gchar *yaml_path = NULL;

  /* Get the f29 and f29-updates indexes. They have multiple streams and
   * versions for the 'dwm' module
   */
  f29 = modulemd_module_index_new ();
  yaml_path = g_strdup_printf ("%s/f29.yaml", g_getenv ("TEST_DATA_PATH"));
  g_assert_true (modulemd_module_index_update_from_file (
    f29, yaml_path, TRUE, &failures, &error));
  g_assert_no_error (error);
  g_assert_cmpint (failures->len, ==, 0);
  g_clear_pointer (&yaml_path, g_free);
  g_clear_pointer (&failures, g_ptr_array_unref);

  f29_updates = modulemd_module_index_new ();
  yaml_path =
    g_strdup_printf ("%s/f29-updates.yaml", g_getenv ("TEST_DATA_PATH"));
  g_assert_true (modulemd_module_index_update_from_file (
    f29_updates, yaml_path, TRUE, &failures, &error));
  g_assert_no_error (error);
  g_assert_cmpint (failures->len, ==, 0);
  g_clear_pointer (&yaml_path, g_free);
  g_clear_pointer (&failures, g_ptr_array_unref);


  /* Merge them so we're operating on a combined index */
  merger = modulemd_module_index_merger_new ();
  modulemd_module_index_merger_associate_index (merger, f29, 0);
  modulemd_module_index_merger_associate_index (merger, f29_updates, 0);

  index = modulemd_module_index_merger_resolve (merger, &error);
  g_assert_nonnull (index);
  g_assert_no_error (error);

  /* Now get the 'nodejs' module */
  nodejs_module = modulemd_module_index_get_module (index, "nodejs");
  g_assert_nonnull (nodejs_module);
  g_assert_true (MODULEMD_IS_MODULE (nodejs_module));

  g_assert_cmpuint (
    modulemd_module_get_all_streams (nodejs_module)->len, ==, 4);

  /* Remove the `nodejs:10:20181101171344:6c81f848:x86_64` item from the
   * index.
   */
  modulemd_module_remove_streams_by_NSVCA (
    nodejs_module, "10", 20181101171344, "6c81f848", "x86_64");

  /* This should remove exactly one item */
  g_assert_cmpuint (
    modulemd_module_get_all_streams (nodejs_module)->len, ==, 3);


  /* Try to remove the same stream from the index a second time, which should
   * do nothing.
   */
  modulemd_module_remove_streams_by_NSVCA (
    nodejs_module, "10", 20181101171344, "6c81f848", "x86_64");

  /* This should remove exactly one item */
  g_assert_cmpuint (
    modulemd_module_get_all_streams (nodejs_module)->len, ==, 3);


  /* Remove all dwm stream objects for the "11" stream from the index. */
  modulemd_module_remove_streams_by_name (nodejs_module, "11");

  /* This should remove two items */
  g_assert_cmpuint (
    modulemd_module_get_all_streams (nodejs_module)->len, ==, 2);
}


int
main (int argc, char *argv[])
{
  setlocale (LC_ALL, "");

  g_test_init (&argc, &argv, NULL);
  g_test_bug_base ("https://bugzilla.redhat.com/show_bug.cgi?id=");

  // Define the tests.

  g_test_add ("/modulemd/v2/module/construct",
              ModuleFixture,
              NULL,
              NULL,
              module_test_construct,
              NULL);

  g_test_add ("/modulemd/v2/module/defaults",
              ModuleFixture,
              NULL,
              NULL,
              module_test_defaults,
              NULL);

  g_test_add ("/modulemd/v2/module/stream_names",
              ModuleFixture,
              NULL,
              NULL,
              module_test_get_stream_names,
              NULL);

  g_test_add ("/modulemd/v2/module/streams",
              ModuleFixture,
              NULL,
              NULL,
              module_test_streams,
              NULL);

  g_test_add_func ("/modulemd/v2/module/streams/remove",
                   modulemd_test_remove_streams);

  return g_test_run ();
}
