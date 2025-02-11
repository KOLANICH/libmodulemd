#include <glib.h>
#include <glib/gstdio.h>
#include <locale.h>
#include <signal.h>

#include "modulemd-module-index.h"
#include "modulemd-module-stream.h"
#include "private/glib-extensions.h"
#include "private/modulemd-module-stream-private.h"
#include "private/modulemd-module-stream-v1-private.h"
#include "private/modulemd-module-stream-v2-private.h"
#include "private/modulemd-subdocument-info-private.h"
#include "private/modulemd-util.h"
#include "private/modulemd-yaml.h"
#include "private/test-utils.h"

typedef struct _ModuleStreamFixture
{
} ModuleStreamFixture;


static void
module_stream_test_construct (ModuleStreamFixture *fixture,
                              gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStream) stream = NULL;
  guint64 version;

  for (version = MD_MODULESTREAM_VERSION_ONE;
       version <= MD_MODULESTREAM_VERSION_LATEST;
       version++)
    {
      /* Test that the new() function works */
      stream = modulemd_module_stream_new (version, "foo", "latest");
      g_assert_nonnull (stream);
      g_assert_true (MODULEMD_IS_MODULE_STREAM (stream));

      g_assert_cmpint (
        modulemd_module_stream_get_mdversion (stream), ==, version);
      g_assert_cmpstr (
        modulemd_module_stream_get_module_name (stream), ==, "foo");
      g_assert_cmpstr (
        modulemd_module_stream_get_stream_name (stream), ==, "latest");

      g_clear_object (&stream);

      /* Test that the new() function works without a stream name */
      stream = modulemd_module_stream_new (version, "foo", NULL);
      g_assert_nonnull (stream);
      g_assert_true (MODULEMD_IS_MODULE_STREAM (stream));

      g_assert_cmpint (
        modulemd_module_stream_get_mdversion (stream), ==, version);
      g_assert_cmpstr (
        modulemd_module_stream_get_module_name (stream), ==, "foo");
      g_assert_null (modulemd_module_stream_get_stream_name (stream));

      g_clear_object (&stream);

      /* Test with no module name */
      stream = modulemd_module_stream_new (version, NULL, NULL);
      g_assert_nonnull (stream);
      g_assert_true (MODULEMD_IS_MODULE_STREAM (stream));

      g_assert_cmpint (
        modulemd_module_stream_get_mdversion (stream), ==, version);
      g_assert_null (modulemd_module_stream_get_module_name (stream));
      g_assert_null (modulemd_module_stream_get_stream_name (stream));

      g_clear_object (&stream);
    }

  /* Test with a zero mdversion */
  stream = modulemd_module_stream_new (0, "foo", "latest");
  g_assert_null (stream);
  g_clear_object (&stream);


  /* Test with an unknown mdversion */
  stream = modulemd_module_stream_new (
    MD_MODULESTREAM_VERSION_LATEST + 1, "foo", "latest");
  g_assert_null (stream);
  g_clear_object (&stream);
}


static void
module_stream_test_arch (ModuleStreamFixture *fixture, gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStream) stream = NULL;
  guint64 version;
  g_autofree gchar *arch = NULL;

  for (version = MD_MODULESTREAM_VERSION_ONE;
       version <= MD_MODULESTREAM_VERSION_LATEST;
       version++)
    {
      /* Test the parent class set_arch() and get_arch() */
      stream = modulemd_module_stream_new (version, "foo", "latest");
      g_assert_nonnull (stream);

      g_assert_null (modulemd_module_stream_get_arch (stream));

      // clang-format off
      g_object_get (stream,
                    "arch", &arch,
                    NULL);
      // clang-format on
      g_assert_null (arch);

      modulemd_module_stream_set_arch (stream, "x86_64");
      g_assert_cmpstr (modulemd_module_stream_get_arch (stream), ==, "x86_64");

      // clang-format off
      g_object_set (stream,
                    "arch", "aarch64",
                    NULL);
      g_object_get (stream,
                    "arch", &arch,
                    NULL);
      // clang-format on
      g_assert_cmpstr (arch, ==, "aarch64");
      g_clear_pointer (&arch, g_free);

      g_clear_object (&stream);
    }
}


static void
module_stream_test_copy (ModuleStreamFixture *fixture, gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStream) stream = NULL;
  g_autoptr (ModulemdModuleStream) copied_stream = NULL;
  guint64 version;

  for (version = MD_MODULESTREAM_VERSION_ONE;
       version <= MD_MODULESTREAM_VERSION_LATEST;
       version++)
    {
      /* Test copying with a stream name */
      stream = modulemd_module_stream_new (version, "foo", "latest");
      copied_stream = modulemd_module_stream_copy (stream, NULL, NULL);
      g_assert_nonnull (copied_stream);
      g_assert_true (MODULEMD_IS_MODULE_STREAM (copied_stream));
      g_assert_cmpstr (modulemd_module_stream_get_module_name (stream),
                       ==,
                       modulemd_module_stream_get_module_name (copied_stream));
      g_assert_cmpstr (modulemd_module_stream_get_stream_name (stream),
                       ==,
                       modulemd_module_stream_get_stream_name (copied_stream));
      g_clear_object (&stream);
      g_clear_object (&copied_stream);


      /* Test copying without a stream name */
      stream = modulemd_module_stream_new (version, "foo", NULL);
      copied_stream = modulemd_module_stream_copy (stream, NULL, NULL);
      g_assert_nonnull (copied_stream);
      g_assert_true (MODULEMD_IS_MODULE_STREAM (copied_stream));
      g_assert_cmpstr (modulemd_module_stream_get_module_name (stream),
                       ==,
                       modulemd_module_stream_get_module_name (copied_stream));
      g_assert_cmpstr (modulemd_module_stream_get_stream_name (stream),
                       ==,
                       modulemd_module_stream_get_stream_name (copied_stream));
      g_clear_object (&stream);
      g_clear_object (&copied_stream);

      /* Test copying with and renaming the stream name */
      stream = modulemd_module_stream_new (version, "foo", "latest");
      copied_stream = modulemd_module_stream_copy (stream, NULL, "earliest");
      g_assert_nonnull (copied_stream);
      g_assert_true (MODULEMD_IS_MODULE_STREAM (copied_stream));
      g_assert_cmpstr (modulemd_module_stream_get_module_name (stream),
                       ==,
                       modulemd_module_stream_get_module_name (copied_stream));
      g_assert_cmpstr (
        modulemd_module_stream_get_stream_name (stream), ==, "latest");
      g_assert_cmpstr (modulemd_module_stream_get_stream_name (copied_stream),
                       ==,
                       "earliest");
      g_clear_object (&stream);
      g_clear_object (&copied_stream);
    }
}


static void
module_stream_test_equals (ModuleStreamFixture *fixture,
                           gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStream) stream_1 = NULL;
  g_autoptr (ModulemdModuleStream) stream_2 = NULL;
  guint64 version;

  for (version = MD_MODULESTREAM_VERSION_ONE;
       version <= MD_MODULESTREAM_VERSION_LATEST;
       version++)
    {
      /* Test equality with same stream and module names */
      stream_1 = modulemd_module_stream_new (version, "foo", "latest");
      stream_2 = modulemd_module_stream_new (version, "foo", "latest");

      g_assert_true (modulemd_module_stream_equals (stream_1, stream_2));
      g_clear_object (&stream_1);
      g_clear_object (&stream_2);


      /* Test equality with different stream names*/
      stream_1 = modulemd_module_stream_new (version, "foo", NULL);
      stream_2 = modulemd_module_stream_new (version, "bar", NULL);

      g_assert_false (modulemd_module_stream_equals (stream_1, stream_2));
      g_clear_object (&stream_1);
      g_clear_object (&stream_2);

      /* Test equality with different module name */
      stream_1 = modulemd_module_stream_new (version, "bar", "thor");
      stream_2 = modulemd_module_stream_new (version, "bar", "loki");

      g_assert_false (modulemd_module_stream_equals (stream_1, stream_2));
      g_clear_object (&stream_1);
      g_clear_object (&stream_2);

      /* Test equality with same arch */
      stream_1 = modulemd_module_stream_new (version, "bar", "thor");
      modulemd_module_stream_set_arch (stream_1, "x86_64");
      stream_2 = modulemd_module_stream_new (version, "bar", "thor");
      modulemd_module_stream_set_arch (stream_2, "x86_64");

      g_assert_true (modulemd_module_stream_equals (stream_1, stream_2));
      g_clear_object (&stream_1);
      g_clear_object (&stream_2);

      /* Test equality with different arch */
      stream_1 = modulemd_module_stream_new (version, "bar", "thor");
      modulemd_module_stream_set_arch (stream_1, "x86_64");
      stream_2 = modulemd_module_stream_new (version, "bar", "thor");
      modulemd_module_stream_set_arch (stream_2, "x86_25");

      g_assert_false (modulemd_module_stream_equals (stream_1, stream_2));
      g_clear_object (&stream_1);
      g_clear_object (&stream_2);
    }
}


G_GNUC_BEGIN_IGNORE_DEPRECATIONS
static void
module_stream_test_nsvc (ModuleStreamFixture *fixture, gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStream) stream = NULL;
  g_autofree gchar *s_nsvc = NULL;
  guint64 version;

  for (version = MD_MODULESTREAM_VERSION_ONE;
       version <= MD_MODULESTREAM_VERSION_LATEST;
       version++)
    {
      // First test that nsvc is None for a module with no name
      stream = modulemd_module_stream_new (version, NULL, NULL);
      s_nsvc = modulemd_module_stream_get_nsvc_as_string (stream);
      g_assert_null (s_nsvc);
      g_clear_pointer (&s_nsvc, g_free);
      g_clear_object (&stream);

      // Now with valid module and stream names
      stream = modulemd_module_stream_new (version, "modulename", NULL);
      s_nsvc = modulemd_module_stream_get_nsvc_as_string (stream);
      g_assert_null (s_nsvc);
      g_clear_pointer (&s_nsvc, g_free);
      g_clear_object (&stream);

      // Now with valid module and stream names
      stream =
        modulemd_module_stream_new (version, "modulename", "streamname");
      s_nsvc = modulemd_module_stream_get_nsvc_as_string (stream);
      g_assert_cmpstr (s_nsvc, ==, "modulename:streamname:0");
      g_clear_pointer (&s_nsvc, g_free);

      //# Add a version number
      modulemd_module_stream_set_version (stream, 42);
      s_nsvc = modulemd_module_stream_get_nsvc_as_string (stream);
      g_assert_cmpstr (s_nsvc, ==, "modulename:streamname:42");
      g_clear_pointer (&s_nsvc, g_free);

      // Add a context
      modulemd_module_stream_set_context (stream, "deadbeef");
      s_nsvc = modulemd_module_stream_get_nsvc_as_string (stream);
      g_assert_cmpstr (s_nsvc, ==, "modulename:streamname:42:deadbeef");
      g_clear_pointer (&s_nsvc, g_free);
      g_clear_object (&stream);
    }
}
G_GNUC_END_IGNORE_DEPRECATIONS


static void
module_stream_test_nsvca (ModuleStreamFixture *fixture,
                          gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStream) stream = NULL;
  g_autofree gchar *s_nsvca = NULL;
  guint64 version;

  for (version = MD_MODULESTREAM_VERSION_ONE;
       version <= MD_MODULESTREAM_VERSION_LATEST;
       version++)
    {
      // First test that NSVCA is None for a module with no name
      stream = modulemd_module_stream_new (version, NULL, NULL);
      s_nsvca = modulemd_module_stream_get_NSVCA_as_string (stream);
      g_assert_null (s_nsvca);
      g_clear_pointer (&s_nsvca, g_free);
      g_clear_object (&stream);

      // Now with valid module and stream names
      stream = modulemd_module_stream_new (version, "modulename", NULL);
      s_nsvca = modulemd_module_stream_get_NSVCA_as_string (stream);
      g_assert_cmpstr (s_nsvca, ==, "modulename");
      g_clear_pointer (&s_nsvca, g_free);
      g_clear_object (&stream);

      // Now with valid module and stream names
      stream =
        modulemd_module_stream_new (version, "modulename", "streamname");
      s_nsvca = modulemd_module_stream_get_NSVCA_as_string (stream);
      g_assert_cmpstr (s_nsvca, ==, "modulename:streamname");
      g_clear_pointer (&s_nsvca, g_free);

      //# Add a version number
      modulemd_module_stream_set_version (stream, 42);
      s_nsvca = modulemd_module_stream_get_NSVCA_as_string (stream);
      g_assert_cmpstr (s_nsvca, ==, "modulename:streamname:42");
      g_clear_pointer (&s_nsvca, g_free);

      // Add a context
      modulemd_module_stream_set_context (stream, "deadbeef");
      s_nsvca = modulemd_module_stream_get_NSVCA_as_string (stream);
      g_assert_cmpstr (s_nsvca, ==, "modulename:streamname:42:deadbeef");
      g_clear_pointer (&s_nsvca, g_free);

      // Add an architecture
      modulemd_module_stream_set_arch (stream, "x86_64");
      s_nsvca = modulemd_module_stream_get_NSVCA_as_string (stream);
      g_assert_cmpstr (
        s_nsvca, ==, "modulename:streamname:42:deadbeef:x86_64");
      g_clear_pointer (&s_nsvca, g_free);

      // Now try removing some of the bits in the middle
      modulemd_module_stream_set_context (stream, NULL);
      s_nsvca = modulemd_module_stream_get_NSVCA_as_string (stream);
      g_assert_cmpstr (s_nsvca, ==, "modulename:streamname:42::x86_64");
      g_clear_pointer (&s_nsvca, g_free);
      g_clear_object (&stream);

      stream = modulemd_module_stream_new (version, "modulename", NULL);
      modulemd_module_stream_set_arch (stream, "x86_64");
      s_nsvca = modulemd_module_stream_get_NSVCA_as_string (stream);
      g_assert_cmpstr (s_nsvca, ==, "modulename::::x86_64");
      g_clear_pointer (&s_nsvca, g_free);

      modulemd_module_stream_set_version (stream, 2019);
      s_nsvca = modulemd_module_stream_get_NSVCA_as_string (stream);
      g_assert_cmpstr (s_nsvca, ==, "modulename::2019::x86_64");
      g_clear_pointer (&s_nsvca, g_free);

      // Add a context
      modulemd_module_stream_set_context (stream, "feedfeed");
      s_nsvca = modulemd_module_stream_get_NSVCA_as_string (stream);
      g_assert_cmpstr (s_nsvca, ==, "modulename::2019:feedfeed:x86_64");
      g_clear_pointer (&s_nsvca, g_free);
      g_clear_object (&stream);
    }
}


static void
module_stream_v1_test_equals (ModuleStreamFixture *fixture,
                              gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStreamV1) stream_1 = NULL;
  g_autoptr (ModulemdModuleStreamV1) stream_2 = NULL;
  g_autoptr (ModulemdProfile) profile_1 = NULL;
  g_autoptr (ModulemdServiceLevel) servicelevel_1 = NULL;
  g_autoptr (ModulemdServiceLevel) servicelevel_2 = NULL;
  g_autoptr (ModulemdComponentModule) component_1 = NULL;
  g_autoptr (ModulemdComponentRpm) component_2 = NULL;

  /*Test equality of 2 streams with same string constants*/
  stream_1 = modulemd_module_stream_v1_new (NULL, NULL);
  modulemd_module_stream_v1_set_community (stream_1, "community_1");
  modulemd_module_stream_v1_set_description (stream_1, "description_1");
  modulemd_module_stream_v1_set_documentation (stream_1, "documentation_1");
  modulemd_module_stream_v1_set_summary (stream_1, "summary_1");
  modulemd_module_stream_v1_set_tracker (stream_1, "tracker_1");

  stream_2 = modulemd_module_stream_v1_new (NULL, NULL);
  modulemd_module_stream_v1_set_community (stream_2, "community_1");
  modulemd_module_stream_v1_set_description (stream_2, "description_1");
  modulemd_module_stream_v1_set_documentation (stream_2, "documentation_1");
  modulemd_module_stream_v1_set_summary (stream_2, "summary_1");
  modulemd_module_stream_v1_set_tracker (stream_2, "tracker_1");

  g_assert_true (modulemd_module_stream_equals (
    (ModulemdModuleStream *)stream_1, (ModulemdModuleStream *)stream_2));
  g_clear_object (&stream_1);
  g_clear_object (&stream_2);

  /*Test equality of 2 streams with certain different string constants*/
  stream_1 = modulemd_module_stream_v1_new (NULL, NULL);
  modulemd_module_stream_v1_set_community (stream_1, "community_1");
  modulemd_module_stream_v1_set_description (stream_1, "description_1");
  modulemd_module_stream_v1_set_documentation (stream_1, "documentation_1");
  modulemd_module_stream_v1_set_summary (stream_1, "summary_1");
  modulemd_module_stream_v1_set_tracker (stream_1, "tracker_1");

  stream_2 = modulemd_module_stream_v1_new (NULL, NULL);
  modulemd_module_stream_v1_set_community (stream_2, "community_1");
  modulemd_module_stream_v1_set_description (stream_2, "description_2");
  modulemd_module_stream_v1_set_documentation (stream_2, "documentation_1");
  modulemd_module_stream_v1_set_summary (stream_2, "summary_2");
  modulemd_module_stream_v1_set_tracker (stream_2, "tracker_2");

  g_assert_false (modulemd_module_stream_equals (
    (ModulemdModuleStream *)stream_1, (ModulemdModuleStream *)stream_2));
  g_clear_object (&stream_1);
  g_clear_object (&stream_2);

  /*Test equality of 2 streams with same hashtable sets*/
  stream_1 = modulemd_module_stream_v1_new (NULL, NULL);
  modulemd_module_stream_v1_add_rpm_api (stream_1, "rpm_1");
  modulemd_module_stream_v1_add_rpm_api (stream_1, "rpm_2");
  modulemd_module_stream_v1_add_module_license (stream_1, "module_a");
  modulemd_module_stream_v1_add_module_license (stream_1, "module_b");
  modulemd_module_stream_v1_add_content_license (stream_1, "content_a");
  modulemd_module_stream_v1_add_content_license (stream_1, "content_b");
  modulemd_module_stream_v1_add_rpm_artifact (stream_1, "artifact_a");
  modulemd_module_stream_v1_add_rpm_artifact (stream_1, "artifact_b");
  modulemd_module_stream_v1_add_rpm_filter (stream_1, "filter_a");
  modulemd_module_stream_v1_add_rpm_filter (stream_1, "filter_b");

  stream_2 = modulemd_module_stream_v1_new (NULL, NULL);
  modulemd_module_stream_v1_add_rpm_api (stream_2, "rpm_1");
  modulemd_module_stream_v1_add_rpm_api (stream_2, "rpm_2");
  modulemd_module_stream_v1_add_module_license (stream_2, "module_a");
  modulemd_module_stream_v1_add_module_license (stream_2, "module_b");
  modulemd_module_stream_v1_add_content_license (stream_2, "content_a");
  modulemd_module_stream_v1_add_content_license (stream_2, "content_b");
  modulemd_module_stream_v1_add_rpm_artifact (stream_2, "artifact_a");
  modulemd_module_stream_v1_add_rpm_artifact (stream_2, "artifact_b");
  modulemd_module_stream_v1_add_rpm_filter (stream_2, "filter_a");
  modulemd_module_stream_v1_add_rpm_filter (stream_2, "filter_b");

  g_assert_true (modulemd_module_stream_equals (
    (ModulemdModuleStream *)stream_1, (ModulemdModuleStream *)stream_2));
  g_clear_object (&stream_1);
  g_clear_object (&stream_2);

  /*Test equality of 2 streams with different hashtable sets*/
  stream_1 = modulemd_module_stream_v1_new (NULL, NULL);
  modulemd_module_stream_v1_add_rpm_api (stream_1, "rpm_1");
  modulemd_module_stream_v1_add_rpm_api (stream_1, "rpm_2");
  modulemd_module_stream_v1_add_module_license (stream_1, "module_a");
  modulemd_module_stream_v1_add_module_license (stream_1, "module_b");
  modulemd_module_stream_v1_add_content_license (stream_1, "content_a");
  modulemd_module_stream_v1_add_content_license (stream_1, "content_b");
  modulemd_module_stream_v1_add_rpm_artifact (stream_1, "artifact_a");
  modulemd_module_stream_v1_add_rpm_artifact (stream_1, "artifact_b");
  modulemd_module_stream_v1_add_rpm_artifact (stream_1, "artifact_c");
  modulemd_module_stream_v1_add_rpm_filter (stream_1, "filter_a");
  modulemd_module_stream_v1_add_rpm_filter (stream_1, "filter_b");

  stream_2 = modulemd_module_stream_v1_new (NULL, NULL);
  modulemd_module_stream_v1_add_rpm_api (stream_2, "rpm_1");
  modulemd_module_stream_v1_add_module_license (stream_2, "module_a");
  modulemd_module_stream_v1_add_module_license (stream_2, "module_b");
  modulemd_module_stream_v1_add_content_license (stream_2, "content_a");
  modulemd_module_stream_v1_add_content_license (stream_2, "content_b");
  modulemd_module_stream_v1_add_rpm_artifact (stream_2, "artifact_a");
  modulemd_module_stream_v1_add_rpm_artifact (stream_2, "artifact_b");
  modulemd_module_stream_v1_add_rpm_filter (stream_2, "filter_a");
  modulemd_module_stream_v1_add_rpm_filter (stream_2, "filter_b");

  g_assert_false (modulemd_module_stream_equals (
    (ModulemdModuleStream *)stream_1, (ModulemdModuleStream *)stream_2));
  g_clear_object (&stream_1);
  g_clear_object (&stream_2);

  /*Test equality of 2 streams with same dependencies*/
  stream_1 = modulemd_module_stream_v1_new (NULL, NULL);
  modulemd_module_stream_v1_add_buildtime_requirement (
    stream_1, "testmodule", "stable");
  modulemd_module_stream_v1_add_runtime_requirement (
    stream_1, "testmodule", "latest");
  stream_2 = modulemd_module_stream_v1_new (NULL, NULL);
  modulemd_module_stream_v1_add_buildtime_requirement (
    stream_2, "testmodule", "stable");
  modulemd_module_stream_v1_add_runtime_requirement (
    stream_2, "testmodule", "latest");

  g_assert_true (modulemd_module_stream_equals (
    (ModulemdModuleStream *)stream_1, (ModulemdModuleStream *)stream_2));
  g_clear_object (&stream_1);
  g_clear_object (&stream_2);

  /*Test equality of 2 streams with different dependencies*/
  stream_1 = modulemd_module_stream_v1_new (NULL, NULL);
  modulemd_module_stream_v1_add_buildtime_requirement (
    stream_1, "test", "stable");
  modulemd_module_stream_v1_add_runtime_requirement (
    stream_1, "testmodule", "latest");
  stream_2 = modulemd_module_stream_v1_new (NULL, NULL);
  modulemd_module_stream_v1_add_buildtime_requirement (
    stream_2, "testmodule", "stable");
  modulemd_module_stream_v1_add_runtime_requirement (
    stream_2, "testmodule", "not_latest");

  g_assert_false (modulemd_module_stream_equals (
    (ModulemdModuleStream *)stream_1, (ModulemdModuleStream *)stream_2));
  g_clear_object (&stream_1);
  g_clear_object (&stream_2);

  /*Test equality of 2 streams with same hashtables*/
  profile_1 = modulemd_profile_new ("testprofile");
  component_1 = modulemd_component_module_new ("testmodule");
  servicelevel_1 = modulemd_service_level_new ("foo");

  stream_1 = modulemd_module_stream_v1_new (NULL, NULL);
  modulemd_module_stream_v1_add_profile (stream_1, profile_1);
  modulemd_module_stream_v1_add_component (stream_1,
                                           (ModulemdComponent *)component_1);
  modulemd_module_stream_v1_add_servicelevel (stream_1, servicelevel_1);
  stream_2 = modulemd_module_stream_v1_new (NULL, NULL);
  modulemd_module_stream_v1_add_profile (stream_2, profile_1);
  modulemd_module_stream_v1_add_component (stream_2,
                                           (ModulemdComponent *)component_1);
  modulemd_module_stream_v1_add_servicelevel (stream_2, servicelevel_1);

  g_assert_true (modulemd_module_stream_equals (
    (ModulemdModuleStream *)stream_1, (ModulemdModuleStream *)stream_2));
  g_clear_object (&stream_1);
  g_clear_object (&stream_2);
  g_clear_object (&profile_1);
  g_clear_object (&component_1);
  g_clear_object (&servicelevel_1);

  /*Test equality of 2 streams with different hashtables*/
  profile_1 = modulemd_profile_new ("testprofile");
  component_1 = modulemd_component_module_new ("testmodule");
  component_2 = modulemd_component_rpm_new ("something");
  servicelevel_1 = modulemd_service_level_new ("foo");
  servicelevel_2 = modulemd_service_level_new ("bar");

  stream_1 = modulemd_module_stream_v1_new (NULL, NULL);
  modulemd_module_stream_v1_add_profile (stream_1, profile_1);
  modulemd_module_stream_v1_add_component (stream_1,
                                           (ModulemdComponent *)component_1);
  modulemd_module_stream_v1_add_servicelevel (stream_1, servicelevel_1);
  stream_2 = modulemd_module_stream_v1_new (NULL, NULL);
  modulemd_module_stream_v1_add_profile (stream_2, profile_1);
  modulemd_module_stream_v1_add_component (stream_2,
                                           (ModulemdComponent *)component_2);
  modulemd_module_stream_v1_add_servicelevel (stream_2, servicelevel_2);

  g_assert_false (modulemd_module_stream_equals (
    (ModulemdModuleStream *)stream_1, (ModulemdModuleStream *)stream_2));
  g_clear_object (&stream_1);
  g_clear_object (&stream_2);
  g_clear_object (&profile_1);
  g_clear_object (&component_1);
  g_clear_object (&component_2);
  g_clear_object (&servicelevel_1);
  g_clear_object (&servicelevel_2);
}


static void
module_stream_v2_test_equals (ModuleStreamFixture *fixture,
                              gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStreamV2) stream_1 = NULL;
  g_autoptr (ModulemdModuleStreamV2) stream_2 = NULL;
  g_autoptr (ModulemdProfile) profile_1 = NULL;
  g_autoptr (ModulemdServiceLevel) servicelevel_1 = NULL;
  g_autoptr (ModulemdServiceLevel) servicelevel_2 = NULL;
  g_autoptr (ModulemdComponentModule) component_1 = NULL;
  g_autoptr (ModulemdComponentRpm) component_2 = NULL;
  g_autoptr (ModulemdDependencies) dep_1 = NULL;
  g_autoptr (ModulemdDependencies) dep_2 = NULL;
  g_autoptr (ModulemdRpmMapEntry) entry_1 = NULL;

  /*Test equality of 2 streams with same string constants*/
  stream_1 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_set_community (stream_1, "community_1");
  modulemd_module_stream_v2_set_description (stream_1, "description_1");
  modulemd_module_stream_v2_set_documentation (stream_1, "documentation_1");
  modulemd_module_stream_v2_set_summary (stream_1, "summary_1");
  modulemd_module_stream_v2_set_tracker (stream_1, "tracker_1");

  stream_2 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_set_community (stream_2, "community_1");
  modulemd_module_stream_v2_set_description (stream_2, "description_1");
  modulemd_module_stream_v2_set_documentation (stream_2, "documentation_1");
  modulemd_module_stream_v2_set_summary (stream_2, "summary_1");
  modulemd_module_stream_v2_set_tracker (stream_2, "tracker_1");

  g_assert_true (modulemd_module_stream_equals (
    (ModulemdModuleStream *)stream_1, (ModulemdModuleStream *)stream_2));
  g_clear_object (&stream_1);
  g_clear_object (&stream_2);

  /*Test equality of 2 streams with certain different string constants*/
  stream_1 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_set_community (stream_1, "community_1");
  modulemd_module_stream_v2_set_description (stream_1, "description_1");
  modulemd_module_stream_v2_set_documentation (stream_1, "documentation_1");
  modulemd_module_stream_v2_set_summary (stream_1, "summary_1");
  modulemd_module_stream_v2_set_tracker (stream_1, "tracker_1");

  stream_2 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_set_community (stream_2, "community_1");
  modulemd_module_stream_v2_set_description (stream_2, "description_2");
  modulemd_module_stream_v2_set_documentation (stream_2, "documentation_1");
  modulemd_module_stream_v2_set_summary (stream_2, "summary_2");
  modulemd_module_stream_v2_set_tracker (stream_2, "tracker_2");

  g_assert_false (modulemd_module_stream_equals (
    (ModulemdModuleStream *)stream_1, (ModulemdModuleStream *)stream_2));
  g_clear_object (&stream_1);
  g_clear_object (&stream_2);

  /*Test equality of 2 streams with same hashtable sets*/
  stream_1 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_add_rpm_api (stream_1, "rpm_1");
  modulemd_module_stream_v2_add_rpm_api (stream_1, "rpm_2");
  modulemd_module_stream_v2_add_module_license (stream_1, "module_a");
  modulemd_module_stream_v2_add_module_license (stream_1, "module_b");
  modulemd_module_stream_v2_add_content_license (stream_1, "content_a");
  modulemd_module_stream_v2_add_content_license (stream_1, "content_b");
  modulemd_module_stream_v2_add_rpm_artifact (stream_1, "artifact_a");
  modulemd_module_stream_v2_add_rpm_artifact (stream_1, "artifact_b");
  modulemd_module_stream_v2_add_rpm_filter (stream_1, "filter_a");
  modulemd_module_stream_v2_add_rpm_filter (stream_1, "filter_b");

  stream_2 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_add_rpm_api (stream_2, "rpm_1");
  modulemd_module_stream_v2_add_rpm_api (stream_2, "rpm_2");
  modulemd_module_stream_v2_add_module_license (stream_2, "module_a");
  modulemd_module_stream_v2_add_module_license (stream_2, "module_b");
  modulemd_module_stream_v2_add_content_license (stream_2, "content_a");
  modulemd_module_stream_v2_add_content_license (stream_2, "content_b");
  modulemd_module_stream_v2_add_rpm_artifact (stream_2, "artifact_a");
  modulemd_module_stream_v2_add_rpm_artifact (stream_2, "artifact_b");
  modulemd_module_stream_v2_add_rpm_filter (stream_2, "filter_a");
  modulemd_module_stream_v2_add_rpm_filter (stream_2, "filter_b");

  g_assert_true (modulemd_module_stream_equals (
    (ModulemdModuleStream *)stream_1, (ModulemdModuleStream *)stream_2));
  g_clear_object (&stream_1);
  g_clear_object (&stream_2);

  /*Test equality of 2 streams with different hashtable sets*/
  stream_1 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_add_rpm_api (stream_1, "rpm_1");
  modulemd_module_stream_v2_add_rpm_api (stream_1, "rpm_2");
  modulemd_module_stream_v2_add_module_license (stream_1, "module_a");
  modulemd_module_stream_v2_add_module_license (stream_1, "module_b");
  modulemd_module_stream_v2_add_content_license (stream_1, "content_a");
  modulemd_module_stream_v2_add_content_license (stream_1, "content_b");
  modulemd_module_stream_v2_add_rpm_artifact (stream_1, "artifact_a");
  modulemd_module_stream_v2_add_rpm_artifact (stream_1, "artifact_b");
  modulemd_module_stream_v2_add_rpm_artifact (stream_1, "artifact_c");
  modulemd_module_stream_v2_add_rpm_filter (stream_1, "filter_a");
  modulemd_module_stream_v2_add_rpm_filter (stream_1, "filter_b");

  stream_2 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_add_rpm_api (stream_2, "rpm_1");
  modulemd_module_stream_v2_add_module_license (stream_2, "module_a");
  modulemd_module_stream_v2_add_module_license (stream_2, "module_b");
  modulemd_module_stream_v2_add_content_license (stream_2, "content_a");
  modulemd_module_stream_v2_add_content_license (stream_2, "content_b");
  modulemd_module_stream_v2_add_rpm_artifact (stream_2, "artifact_a");
  modulemd_module_stream_v2_add_rpm_artifact (stream_2, "artifact_b");
  modulemd_module_stream_v2_add_rpm_filter (stream_2, "filter_a");
  modulemd_module_stream_v2_add_rpm_filter (stream_2, "filter_b");

  g_assert_false (modulemd_module_stream_equals (
    (ModulemdModuleStream *)stream_1, (ModulemdModuleStream *)stream_2));
  g_clear_object (&stream_1);
  g_clear_object (&stream_2);

  /*Test equality of 2 streams with same hashtables*/
  profile_1 = modulemd_profile_new ("testprofile");
  component_1 = modulemd_component_module_new ("testmodule");
  servicelevel_1 = modulemd_service_level_new ("foo");

  stream_1 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_add_profile (stream_1, profile_1);
  modulemd_module_stream_v2_add_component (stream_1,
                                           (ModulemdComponent *)component_1);
  modulemd_module_stream_v2_add_servicelevel (stream_1, servicelevel_1);
  stream_2 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_add_profile (stream_2, profile_1);
  modulemd_module_stream_v2_add_component (stream_2,
                                           (ModulemdComponent *)component_1);
  modulemd_module_stream_v2_add_servicelevel (stream_2, servicelevel_1);

  g_assert_true (modulemd_module_stream_equals (
    (ModulemdModuleStream *)stream_1, (ModulemdModuleStream *)stream_2));
  g_clear_object (&stream_1);
  g_clear_object (&stream_2);
  g_clear_object (&profile_1);
  g_clear_object (&component_1);
  g_clear_object (&servicelevel_1);

  /*Test equality of 2 streams with different hashtables*/
  profile_1 = modulemd_profile_new ("testprofile");
  component_1 = modulemd_component_module_new ("testmodule");
  component_2 = modulemd_component_rpm_new ("something");
  servicelevel_1 = modulemd_service_level_new ("foo");
  servicelevel_2 = modulemd_service_level_new ("bar");

  stream_1 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_add_profile (stream_1, profile_1);
  modulemd_module_stream_v2_add_component (stream_1,
                                           (ModulemdComponent *)component_1);
  modulemd_module_stream_v2_add_servicelevel (stream_1, servicelevel_1);
  stream_2 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_add_profile (stream_2, profile_1);
  modulemd_module_stream_v2_add_component (stream_2,
                                           (ModulemdComponent *)component_2);
  modulemd_module_stream_v2_add_servicelevel (stream_2, servicelevel_2);

  g_assert_false (modulemd_module_stream_equals (
    (ModulemdModuleStream *)stream_1, (ModulemdModuleStream *)stream_2));
  g_clear_object (&stream_1);
  g_clear_object (&stream_2);
  g_clear_object (&profile_1);
  g_clear_object (&component_1);
  g_clear_object (&component_2);
  g_clear_object (&servicelevel_1);
  g_clear_object (&servicelevel_2);

  /*Test equality of 2 streams with same dependencies*/
  dep_1 = modulemd_dependencies_new ();
  modulemd_dependencies_add_buildtime_stream (dep_1, "foo", "stable");

  stream_1 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_add_dependencies (stream_1, dep_1);
  stream_2 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_add_dependencies (stream_2, dep_1);

  g_assert_true (modulemd_module_stream_equals (
    (ModulemdModuleStream *)stream_1, (ModulemdModuleStream *)stream_2));
  g_clear_object (&stream_1);
  g_clear_object (&stream_2);
  g_clear_object (&dep_1);

  /*Test equality of 2 streams with different dependencies*/
  dep_1 = modulemd_dependencies_new ();
  modulemd_dependencies_add_buildtime_stream (dep_1, "foo", "stable");
  dep_2 = modulemd_dependencies_new ();
  modulemd_dependencies_add_buildtime_stream (dep_2, "foo", "latest");

  stream_1 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_add_dependencies (stream_1, dep_1);
  stream_2 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_add_dependencies (stream_2, dep_2);

  g_assert_false (modulemd_module_stream_equals (
    (ModulemdModuleStream *)stream_1, (ModulemdModuleStream *)stream_2));
  g_clear_object (&stream_1);
  g_clear_object (&stream_2);
  g_clear_object (&dep_1);
  g_clear_object (&dep_2);

  /*Test equality of 2 streams with same rpm artifact map entry*/
  entry_1 = modulemd_rpm_map_entry_new (
    "bar", 0, "1.23", "1.module_deadbeef", "x86_64");

  stream_1 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_set_rpm_artifact_map_entry (
    stream_1, entry_1, "sha256", "baddad");
  stream_2 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_set_rpm_artifact_map_entry (
    stream_2, entry_1, "sha256", "baddad");

  g_assert_true (modulemd_module_stream_equals (
    (ModulemdModuleStream *)stream_1, (ModulemdModuleStream *)stream_2));
  g_clear_object (&stream_1);
  g_clear_object (&stream_2);
  g_clear_object (&entry_1);

  /*Test equality of 2 streams with different rpm artifact map entry*/
  entry_1 = modulemd_rpm_map_entry_new (
    "bar", 0, "1.23", "1.module_deadbeef", "x86_64");

  stream_1 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_set_rpm_artifact_map_entry (
    stream_1, entry_1, "sha256", "baddad");
  stream_2 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_set_rpm_artifact_map_entry (
    stream_2, entry_1, "sha256", "badmom");

  g_assert_false (modulemd_module_stream_equals (
    (ModulemdModuleStream *)stream_1, (ModulemdModuleStream *)stream_2));
  g_clear_object (&stream_1);
  g_clear_object (&stream_2);
  g_clear_object (&entry_1);
}


static void
module_stream_v1_test_dependencies (ModuleStreamFixture *fixture,
                                    gconstpointer user_data)
{
  g_auto (GStrv) list = NULL;
  g_autoptr (ModulemdModuleStreamV1) stream = NULL;
  stream = modulemd_module_stream_v1_new (NULL, NULL);
  modulemd_module_stream_v1_add_buildtime_requirement (
    stream, "testmodule", "stable");
  list = modulemd_module_stream_v1_get_buildtime_modules_as_strv (stream);
  g_assert_cmpint (g_strv_length (list), ==, 1);
  g_assert_cmpstr (list[0], ==, "testmodule");
  g_assert_cmpstr (modulemd_module_stream_v1_get_buildtime_requirement_stream (
                     stream, "testmodule"),
                   ==,
                   "stable");
  g_clear_pointer (&list, g_strfreev);

  modulemd_module_stream_v1_add_runtime_requirement (
    stream, "testmodule", "latest");
  list = modulemd_module_stream_v1_get_runtime_modules_as_strv (stream);
  g_assert_cmpint (g_strv_length (list), ==, 1);
  g_assert_cmpstr (list[0], ==, "testmodule");
  g_assert_cmpstr (modulemd_module_stream_v1_get_runtime_requirement_stream (
                     stream, "testmodule"),
                   ==,
                   "latest");
  g_clear_pointer (&list, g_strfreev);
  g_clear_object (&stream);
}


static void
module_stream_v2_test_dependencies (ModuleStreamFixture *fixture,
                                    gconstpointer user_data)
{
  g_auto (GStrv) list = NULL;
  g_autoptr (ModulemdModuleStreamV2) stream = NULL;
  g_autoptr (ModulemdDependencies) dep = NULL;
  stream = modulemd_module_stream_v2_new (NULL, NULL);
  dep = modulemd_dependencies_new ();
  modulemd_dependencies_add_buildtime_stream (dep, "foo", "stable");
  modulemd_dependencies_set_empty_runtime_dependencies_for_module (dep, "bar");
  modulemd_module_stream_v2_add_dependencies (stream, dep);
  GPtrArray *deps_list = modulemd_module_stream_v2_get_dependencies (stream);
  g_assert_cmpint (deps_list->len, ==, 1);

  list = modulemd_dependencies_get_buildtime_modules_as_strv (
    g_ptr_array_index (deps_list, 0));
  g_assert_cmpstr (list[0], ==, "foo");
  g_clear_pointer (&list, g_strfreev);

  list = modulemd_dependencies_get_buildtime_streams_as_strv (
    g_ptr_array_index (deps_list, 0), "foo");
  g_assert_nonnull (list);
  g_assert_cmpstr (list[0], ==, "stable");
  g_assert_null (list[1]);
  g_clear_pointer (&list, g_strfreev);

  list = modulemd_dependencies_get_runtime_modules_as_strv (
    g_ptr_array_index (deps_list, 0));
  g_assert_nonnull (list);
  g_assert_cmpstr (list[0], ==, "bar");
  g_assert_null (list[1]);

  g_clear_pointer (&list, g_strfreev);
  g_clear_object (&dep);
  g_clear_object (&stream);
}


static void
module_stream_v1_test_parse_dump (ModuleStreamFixture *fixture,
                                  gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStreamV1) stream = NULL;
  g_autoptr (GError) error = NULL;
  gboolean ret;
  MMD_INIT_YAML_PARSER (parser);
  MMD_INIT_YAML_EVENT (event);
  MMD_INIT_YAML_EMITTER (emitter);
  MMD_INIT_YAML_STRING (&emitter, yaml_string);
  g_autofree gchar *yaml_path = NULL;
  g_autoptr (FILE) yaml_stream = NULL;
  g_autoptr (ModulemdSubdocumentInfo) subdoc = NULL;
  yaml_path =
    g_strdup_printf ("%s/spec.v1.yaml", g_getenv ("MESON_SOURCE_ROOT"));
  g_assert_nonnull (yaml_path);

  yaml_stream = g_fopen (yaml_path, "rb");
  g_assert_nonnull (yaml_stream);

  /* First parse it */
  yaml_parser_set_input_file (&parser, yaml_stream);
  g_assert_true (yaml_parser_parse (&parser, &event));
  g_assert_cmpint (event.type, ==, YAML_STREAM_START_EVENT);
  yaml_event_delete (&event);
  g_assert_true (yaml_parser_parse (&parser, &event));
  g_assert_cmpint (event.type, ==, YAML_DOCUMENT_START_EVENT);
  yaml_event_delete (&event);

  subdoc = modulemd_yaml_parse_document_type (&parser);
  g_assert_nonnull (subdoc);
  g_assert_null (modulemd_subdocument_info_get_gerror (subdoc));

  g_assert_cmpint (modulemd_subdocument_info_get_doctype (subdoc),
                   ==,
                   MODULEMD_YAML_DOC_MODULESTREAM);
  g_assert_cmpint (modulemd_subdocument_info_get_mdversion (subdoc), ==, 1);
  g_assert_nonnull (modulemd_subdocument_info_get_yaml (subdoc));

  stream = modulemd_module_stream_v1_parse_yaml (subdoc, TRUE, &error);
  g_assert_no_error (error);
  g_assert_nonnull (stream);

  /* Then dump it */
  g_debug ("Starting dumping");
  g_assert_true (mmd_emitter_start_stream (&emitter, &error));
  ret = modulemd_module_stream_v1_emit_yaml (stream, &emitter, &error);
  g_assert_no_error (error);
  g_assert_true (ret);
  ret = mmd_emitter_end_stream (&emitter, &error);
  g_assert_no_error (error);
  g_assert_true (ret);
  g_assert_nonnull (yaml_string->str);

  g_assert_cmpstr (
    yaml_string->str,
    ==,
    "---\n"
    "document: modulemd\n"
    "version: 1\n"
    "data:\n"
    "  name: foo\n"
    "  stream: stream-name\n"
    "  version: 20160927144203\n"
    "  context: c0ffee43\n"
    "  arch: x86_64\n"
    "  summary: An example module\n"
    "  description: >-\n"
    "    A module for the demonstration of the metadata format. Also, the "
    "obligatory lorem\n"
    "    ipsum dolor sit amet goes right here.\n"
    "  servicelevels:\n"
    "    bug_fixes:\n"
    "      eol: 2077-10-23\n"
    "    rawhide:\n"
    "      eol: 2077-10-23\n"
    "    security_fixes:\n"
    "      eol: 2077-10-23\n"
    "    stable_api:\n"
    "      eol: 2077-10-23\n"
    "  license:\n"
    "    module:\n"
    "    - MIT\n"
    "    content:\n"
    "    - Beerware\n"
    "    - GPLv2+\n"
    "    - zlib\n"
    "  xmd:\n"
    "    some_key: some_data\n"
    "  dependencies:\n"
    "    buildrequires:\n"
    "      extra-build-env: and-its-stream-name-too\n"
    "      platform: and-its-stream-name\n"
    "    requires:\n"
    "      platform: and-its-stream-name\n"
    "  references:\n"
    "    community: http://www.example.com/\n"
    "    documentation: http://www.example.com/\n"
    "    tracker: http://www.example.com/\n"
    "  profiles:\n"
    "    buildroot:\n"
    "      rpms:\n"
    "      - bar-devel\n"
    "    container:\n"
    "      rpms:\n"
    "      - bar\n"
    "      - bar-devel\n"
    "    default:\n"
    "      rpms:\n"
    "      - bar\n"
    "      - bar-extras\n"
    "      - baz\n"
    "    minimal:\n"
    "      description: Minimal profile installing only the bar package.\n"
    "      rpms:\n"
    "      - bar\n"
    "    srpm-buildroot:\n"
    "      rpms:\n"
    "      - bar-extras\n"
    "  api:\n"
    "    rpms:\n"
    "    - bar\n"
    "    - bar-devel\n"
    "    - bar-extras\n"
    "    - baz\n"
    "    - xxx\n"
    "  filter:\n"
    "    rpms:\n"
    "    - baz-nonfoo\n"
    "  buildopts:\n"
    "    rpms:\n"
    "      macros: >\n"
    "        %demomacro 1\n"
    "\n"
    "        %demomacro2 %{demomacro}23\n"
    "  components:\n"
    "    rpms:\n"
    "      bar:\n"
    "        rationale: We need this to demonstrate stuff.\n"
    "        repository: https://pagure.io/bar.git\n"
    "        cache: https://example.com/cache\n"
    "        ref: 26ca0c0\n"
    "      baz:\n"
    "        rationale: This one is here to demonstrate other stuff.\n"
    "      xxx:\n"
    "        rationale: xxx demonstrates arches and multilib.\n"
    "        arches: [i686, x86_64]\n"
    "        multilib: [x86_64]\n"
    "      xyz:\n"
    "        rationale: xyz is a bundled dependency of xxx.\n"
    "        buildorder: 10\n"
    "    modules:\n"
    "      includedmodule:\n"
    "        rationale: Included in the stack, just because.\n"
    "        repository: https://pagure.io/includedmodule.git\n"
    "        ref: somecoolbranchname\n"
    "        buildorder: 100\n"
    "  artifacts:\n"
    "    rpms:\n"
    "    - bar-0:1.23-1.module_deadbeef.x86_64\n"
    "    - bar-devel-0:1.23-1.module_deadbeef.x86_64\n"
    "    - bar-extras-0:1.23-1.module_deadbeef.x86_64\n"
    "    - baz-0:42-42.module_deadbeef.x86_64\n"
    "    - xxx-0:1-1.module_deadbeef.i686\n"
    "    - xxx-0:1-1.module_deadbeef.x86_64\n"
    "    - xyz-0:1-1.module_deadbeef.x86_64\n"
    "...\n");
}

static void
module_stream_v2_test_parse_dump (ModuleStreamFixture *fixture,
                                  gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStreamV2) stream = NULL;
  g_autoptr (GError) error = NULL;
  gboolean ret;
  MMD_INIT_YAML_PARSER (parser);
  MMD_INIT_YAML_EVENT (event);
  MMD_INIT_YAML_EMITTER (emitter);
  MMD_INIT_YAML_STRING (&emitter, yaml_string);
  g_autofree gchar *yaml_path = NULL;
  g_autoptr (FILE) yaml_stream = NULL;
  g_autoptr (ModulemdSubdocumentInfo) subdoc = NULL;
  yaml_path =
    g_strdup_printf ("%s/spec.v2.yaml", g_getenv ("MESON_SOURCE_ROOT"));
  g_assert_nonnull (yaml_path);

  yaml_stream = g_fopen (yaml_path, "rb");
  g_assert_nonnull (yaml_stream);

  /* First parse it */
  yaml_parser_set_input_file (&parser, yaml_stream);
  g_assert_true (yaml_parser_parse (&parser, &event));
  g_assert_cmpint (event.type, ==, YAML_STREAM_START_EVENT);
  yaml_event_delete (&event);
  g_assert_true (yaml_parser_parse (&parser, &event));
  g_assert_cmpint (event.type, ==, YAML_DOCUMENT_START_EVENT);
  yaml_event_delete (&event);

  subdoc = modulemd_yaml_parse_document_type (&parser);
  g_assert_nonnull (subdoc);
  g_assert_null (modulemd_subdocument_info_get_gerror (subdoc));

  g_assert_cmpint (modulemd_subdocument_info_get_doctype (subdoc),
                   ==,
                   MODULEMD_YAML_DOC_MODULESTREAM);
  g_assert_cmpint (modulemd_subdocument_info_get_mdversion (subdoc), ==, 2);
  g_assert_nonnull (modulemd_subdocument_info_get_yaml (subdoc));

  stream = modulemd_module_stream_v2_parse_yaml (subdoc, TRUE, &error);
  g_assert_no_error (error);
  g_assert_nonnull (stream);

  /* Then dump it */
  g_debug ("Starting dumping");
  g_assert_true (mmd_emitter_start_stream (&emitter, &error));
  ret = modulemd_module_stream_v2_emit_yaml (stream, &emitter, &error);
  g_assert_no_error (error);
  g_assert_true (ret);
  ret = mmd_emitter_end_stream (&emitter, &error);
  g_assert_no_error (error);
  g_assert_true (ret);
  g_assert_nonnull (yaml_string->str);

  g_assert_cmpstr (
    yaml_string->str,
    ==,
    "---\n"
    "document: modulemd\n"
    "version: 2\n"
    "data:\n"
    "  name: foo\n"
    "  stream: latest\n"
    "  version: 20160927144203\n"
    "  context: c0ffee43\n"
    "  arch: x86_64\n"
    "  summary: An example module\n"
    "  description: >-\n"
    "    A module for the demonstration of the metadata format. Also, the "
    "obligatory lorem\n"
    "    ipsum dolor sit amet goes right here.\n"
    "  servicelevels:\n"
    "    bug_fixes:\n"
    "      eol: 2077-10-23\n"
    "    rawhide:\n"
    "      eol: 2077-10-23\n"
    "    security_fixes:\n"
    "      eol: 2077-10-23\n"
    "    stable_api:\n"
    "      eol: 2077-10-23\n"
    "  license:\n"
    "    module:\n"
    "    - MIT\n"
    "    content:\n"
    "    - Beerware\n"
    "    - GPLv2+\n"
    "    - zlib\n"
    "  xmd:\n"
    "    some_key: some_data\n"
    "  dependencies:\n"
    "  - buildrequires:\n"
    "      platform: [-epel7, -f27, -f28]\n"
    "    requires:\n"
    "      platform: [-epel7, -f27, -f28]\n"
    "  - buildrequires:\n"
    "      buildtools: [v1, v2]\n"
    "      compatible: [v3]\n"
    "      platform: [f27]\n"
    "    requires:\n"
    "      compatible: [v3, v4]\n"
    "      platform: [f27]\n"
    "  - buildrequires:\n"
    "      platform: [f28]\n"
    "    requires:\n"
    "      platform: [f28]\n"
    "      runtime: [a, b]\n"
    "  - buildrequires:\n"
    "      extras: []\n"
    "      moreextras: [bar, foo]\n"
    "      platform: [epel7]\n"
    "    requires:\n"
    "      extras: []\n"
    "      moreextras: [bar, foo]\n"
    "      platform: [epel7]\n"
    "  references:\n"
    "    community: http://www.example.com/\n"
    "    documentation: http://www.example.com/\n"
    "    tracker: http://www.example.com/\n"
    "  profiles:\n"
    "    buildroot:\n"
    "      rpms:\n"
    "      - bar-devel\n"
    "    container:\n"
    "      rpms:\n"
    "      - bar\n"
    "      - bar-devel\n"
    "    default:\n"
    "      rpms:\n"
    "      - bar\n"
    "      - bar-extras\n"
    "      - baz\n"
    "    minimal:\n"
    "      description: Minimal profile installing only the bar package.\n"
    "      rpms:\n"
    "      - bar\n"
    "    srpm-buildroot:\n"
    "      rpms:\n"
    "      - bar-extras\n"
    "  api:\n"
    "    rpms:\n"
    "    - bar\n"
    "    - bar-devel\n"
    "    - bar-extras\n"
    "    - baz\n"
    "    - xxx\n"
    "  filter:\n"
    "    rpms:\n"
    "    - baz-nonfoo\n"
    "  buildopts:\n"
    "    rpms:\n"
    "      macros: >\n"
    "        %demomacro 1\n"
    "\n"
    "        %demomacro2 %{demomacro}23\n"
    "      whitelist:\n"
    "      - fooscl-1-bar\n"
    "      - fooscl-1-baz\n"
    "      - xxx\n"
    "      - xyz\n"
    "  components:\n"
    "    rpms:\n"
    "      bar:\n"
    "        rationale: We need this to demonstrate stuff.\n"
    "        name: bar-real\n"
    "        repository: https://pagure.io/bar.git\n"
    "        cache: https://example.com/cache\n"
    "        ref: 26ca0c0\n"
    "      baz:\n"
    "        rationale: This one is here to demonstrate other stuff.\n"
    "      xxx:\n"
    "        rationale: xxx demonstrates arches and multilib.\n"
    "        arches: [i686, x86_64]\n"
    "        multilib: [x86_64]\n"
    "      xyz:\n"
    "        rationale: xyz is a bundled dependency of xxx.\n"
    "        buildorder: 10\n"
    "    modules:\n"
    "      includedmodule:\n"
    "        rationale: Included in the stack, just because.\n"
    "        repository: https://pagure.io/includedmodule.git\n"
    "        ref: somecoolbranchname\n"
    "        buildorder: 100\n"
    "  artifacts:\n"
    "    rpms:\n"
    "    - bar-0:1.23-1.module_deadbeef.x86_64\n"
    "    - bar-devel-0:1.23-1.module_deadbeef.x86_64\n"
    "    - bar-extras-0:1.23-1.module_deadbeef.x86_64\n"
    "    - baz-0:42-42.module_deadbeef.x86_64\n"
    "    - xxx-0:1-1.module_deadbeef.i686\n"
    "    - xxx-0:1-1.module_deadbeef.x86_64\n"
    "    - xyz-0:1-1.module_deadbeef.x86_64\n"
    "    rpm-map:\n"
    "      sha256:\n"
    "        "
    "ee47083ed80146eb2c84e9a94d0836393912185dcda62b9d93ee0c2ea5dc795b:\n"
    "          name: bar\n"
    "          epoch: 0\n"
    "          version: 1.23\n"
    "          release: 1.module_deadbeef\n"
    "          arch: x86_64\n"
    "          nevra: bar-0:1.23-1.module_deadbeef.x86_64\n"
    "...\n");
}

static void
module_stream_v1_test_depends_on_stream (ModuleStreamFixture *fixture,
                                         gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStream) stream = NULL;
  g_autofree gchar *path = NULL;
  g_autoptr (GError) error = NULL;
  g_autofree gchar *module_name = NULL;
  g_autofree gchar *module_stream = NULL;

  path = g_strdup_printf ("%s/dependson_v1.yaml", g_getenv ("TEST_DATA_PATH"));
  g_assert_nonnull (path);
  stream = modulemd_module_stream_read_file (
    path, TRUE, module_name, module_stream, &error);
  g_assert_nonnull (stream);

  g_assert_true (
    modulemd_module_stream_depends_on_stream (stream, "platform", "f30"));
  g_assert_true (modulemd_module_stream_build_depends_on_stream (
    stream, "platform", "f30"));

  g_assert_false (
    modulemd_module_stream_depends_on_stream (stream, "platform", "f28"));
  g_assert_false (modulemd_module_stream_build_depends_on_stream (
    stream, "platform", "f28"));

  g_assert_false (
    modulemd_module_stream_depends_on_stream (stream, "base", "f30"));
  g_assert_false (
    modulemd_module_stream_build_depends_on_stream (stream, "base", "f30"));
  g_clear_object (&stream);
}

static void
module_stream_v2_test_depends_on_stream (ModuleStreamFixture *fixture,
                                         gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStream) stream = NULL;
  g_autofree gchar *path = NULL;
  g_autoptr (GError) error = NULL;
  g_autofree gchar *module_name = NULL;
  g_autofree gchar *module_stream = NULL;

  path = g_strdup_printf ("%s/dependson_v2.yaml", g_getenv ("TEST_DATA_PATH"));
  g_assert_nonnull (path);
  stream = modulemd_module_stream_read_file (
    path, TRUE, module_name, module_stream, &error);
  g_assert_nonnull (stream);

  g_assert_true (
    modulemd_module_stream_depends_on_stream (stream, "platform", "f30"));
  g_assert_true (modulemd_module_stream_build_depends_on_stream (
    stream, "platform", "f30"));

  g_assert_false (
    modulemd_module_stream_depends_on_stream (stream, "platform", "f28"));
  g_assert_false (modulemd_module_stream_build_depends_on_stream (
    stream, "platform", "f28"));

  g_assert_false (
    modulemd_module_stream_depends_on_stream (stream, "base", "f30"));
  g_assert_false (
    modulemd_module_stream_build_depends_on_stream (stream, "base", "f30"));
  g_clear_object (&stream);
}

static void
module_stream_v2_test_validate_buildafter (ModuleStreamFixture *fixture,
                                           gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStream) stream = NULL;
  g_autofree gchar *path = NULL;
  g_autoptr (GError) error = NULL;

  /* Test a valid module stream with buildafter set */
  path = g_strdup_printf ("%s/buildafter/good_buildafter.yaml",
                          g_getenv ("TEST_DATA_PATH"));
  g_assert_nonnull (path);
  stream = modulemd_module_stream_read_file (path, TRUE, NULL, NULL, &error);
  g_assert_nonnull (stream);
  g_assert_null (error);
  g_clear_pointer (&path, g_free);
  g_clear_object (&stream);

  /* Should fail validation if both buildorder and buildafter are set for the
   * same component.
   */
  path = g_strdup_printf ("%s/buildafter/both_same_component.yaml",
                          g_getenv ("TEST_DATA_PATH"));
  g_assert_nonnull (path);
  stream = modulemd_module_stream_read_file (path, TRUE, NULL, NULL, &error);
  g_assert_error (error, MODULEMD_ERROR, MODULEMD_ERROR_VALIDATE);
  g_assert_null (stream);
  g_clear_error (&error);
  g_clear_pointer (&path, g_free);

  /* Should fail validation if both buildorder and buildafter are set in
   * different components of the same stream.
   */
  path = g_strdup_printf ("%s/buildafter/mixed_buildorder.yaml",
                          g_getenv ("TEST_DATA_PATH"));
  g_assert_nonnull (path);
  stream = modulemd_module_stream_read_file (path, TRUE, NULL, NULL, &error);
  g_assert_error (error, MODULEMD_ERROR, MODULEMD_ERROR_VALIDATE);
  g_assert_null (stream);
  g_clear_error (&error);
  g_clear_pointer (&path, g_free);

  /* Should fail if a key specified in a buildafter set does not exist for this
   * module stream.
   */
  path = g_strdup_printf ("%s/buildafter/invalid_key.yaml",
                          g_getenv ("TEST_DATA_PATH"));
  g_assert_nonnull (path);
  stream = modulemd_module_stream_read_file (path, TRUE, NULL, NULL, &error);
  g_assert_error (error, MODULEMD_ERROR, MODULEMD_ERROR_VALIDATE);
  g_assert_null (stream);
  g_clear_error (&error);
  g_clear_pointer (&path, g_free);
}


static void
module_stream_v2_test_rpm_map (ModuleStreamFixture *fixture,
                               gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStreamV2) stream = NULL;
  g_autoptr (ModulemdRpmMapEntry) entry = NULL;
  ModulemdRpmMapEntry *retrieved_entry = NULL;

  stream = modulemd_module_stream_v2_new ("foo", "bar");
  g_assert_nonnull (stream);

  entry = modulemd_rpm_map_entry_new (
    "bar", 0, "1.23", "1.module_deadbeef", "x86_64");
  g_assert_nonnull (entry);

  modulemd_module_stream_v2_set_rpm_artifact_map_entry (
    stream, entry, "sha256", "baddad");

  retrieved_entry = modulemd_module_stream_v2_get_rpm_artifact_map_entry (
    stream, "sha256", "baddad");
  g_assert_nonnull (retrieved_entry);

  g_assert_true (modulemd_rpm_map_entry_equals (entry, retrieved_entry));
}

static void
module_stream_v2_test_unicode_desc (void)
{
  g_autoptr (ModulemdModuleStream) stream = NULL;
  g_autofree gchar *path = NULL;
  g_autoptr (GError) error = NULL;

  /* Test a module stream with unicode in description */
  path =
    g_strdup_printf ("%s/stream_unicode.yaml", g_getenv ("TEST_DATA_PATH"));
  g_assert_nonnull (path);

  stream = modulemd_module_stream_read_file (path, TRUE, NULL, NULL, &error);
  g_assert_nonnull (stream);
  g_assert_no_error (error);
}


static void
module_stream_v2_test_xmd_issue_274 (void)
{
  g_autoptr (ModulemdModuleStream) stream = NULL;
  g_autofree gchar *path = NULL;
  g_autoptr (GError) error = NULL;
  GVariant *xmd1 = NULL;
  GVariant *xmd2 = NULL;

  path =
    g_strdup_printf ("%s/stream_unicode.yaml", g_getenv ("TEST_DATA_PATH"));
  g_assert_nonnull (path);

  stream = modulemd_module_stream_read_file (path, TRUE, NULL, NULL, &error);
  g_assert_nonnull (stream);
  g_assert_no_error (error);
  g_assert_cmpint (modulemd_module_stream_get_mdversion (stream),
                   ==,
                   MD_MODULESTREAM_VERSION_ONE);

  xmd1 =
    modulemd_module_stream_v1_get_xmd (MODULEMD_MODULE_STREAM_V1 (stream));
  xmd2 =
    modulemd_module_stream_v1_get_xmd (MODULEMD_MODULE_STREAM_V1 (stream));

  g_assert_true (xmd1 == xmd2);
}


static void
module_stream_v2_test_xmd_issue_290 (void)
{
  g_auto (GVariantBuilder) builder;
  g_autoptr (GVariant) xmd = NULL;
  GVariant *xmd_array = NULL;
  g_autoptr (GVariantDict) xmd_dict = NULL;
  g_autoptr (GError) error = NULL;
  g_autoptr (ModulemdModuleIndex) index = modulemd_module_index_new ();
  g_autofree gchar *yaml_str = NULL;

  g_autoptr (ModulemdModuleStreamV2) stream =
    modulemd_module_stream_v2_new ("foo", "bar");

  modulemd_module_stream_v2_set_summary (stream, "summary");
  modulemd_module_stream_v2_set_description (stream, "desc");
  modulemd_module_stream_v2_add_module_license (stream, "MIT");


  g_variant_builder_init (&builder, G_VARIANT_TYPE_ARRAY);

  g_variant_builder_add_value (&builder, g_variant_new_string ("foo"));
  g_variant_builder_add_value (&builder, g_variant_new_string ("bar"));

  xmd_array = g_variant_builder_end (&builder);

  xmd_dict = g_variant_dict_new (NULL);
  g_variant_dict_insert_value (
    xmd_dict, "something", g_steal_pointer (&xmd_array));
  xmd = g_variant_ref_sink (g_variant_dict_end (xmd_dict));


  modulemd_module_stream_v2_set_xmd (stream, xmd);

  g_assert_true (modulemd_module_index_add_module_stream (
    index, MODULEMD_MODULE_STREAM (stream), &error));
  g_assert_no_error (error);

  yaml_str = modulemd_module_index_dump_to_string (index, &error);

  // clang-format off
  g_assert_cmpstr (yaml_str, ==,
"---\n"
"document: modulemd\n"
"version: 2\n"
"data:\n"
"  name: foo\n"
"  stream: bar\n"
"  summary: summary\n"
"  description: >-\n"
"    desc\n"
"  license:\n"
"    module:\n"
"    - MIT\n"
"  xmd:\n"
"    something:\n"
"    - foo\n"
"    - bar\n"
"...\n");
  // clang-format on

  g_assert_no_error (error);
}


static void
module_stream_v2_test_xmd_issue_290_with_example (void)
{
  g_autoptr (ModulemdModuleStream) stream = NULL;
  g_autofree gchar *path = NULL;
  g_autoptr (GError) error = NULL;
  g_autoptr (ModulemdModuleIndex) index = modulemd_module_index_new ();
  g_autofree gchar *output_yaml = NULL;
  g_autoptr (GVariant) xmd = NULL;

  path = g_strdup_printf ("%s/290.yaml", g_getenv ("TEST_DATA_PATH"));
  g_assert_nonnull (path);
  stream = modulemd_module_stream_read_file (path, TRUE, NULL, NULL, &error);
  g_assert_nonnull (stream);
  g_assert_no_error (error);

  xmd = modulemd_variant_deep_copy (
    modulemd_module_stream_v1_get_xmd (MODULEMD_MODULE_STREAM_V1 (stream)));
  modulemd_module_stream_v1_set_xmd (MODULEMD_MODULE_STREAM_V1 (stream), xmd);

  g_assert_true (
    modulemd_module_index_add_module_stream (index, stream, &error));
  g_assert_no_error (error);

  output_yaml = modulemd_module_index_dump_to_string (index, &error);
  g_assert_nonnull (output_yaml);
  g_assert_no_error (error);
}


int
main (int argc, char *argv[])
{
  setlocale (LC_ALL, "");
  g_test_init (&argc, &argv, NULL);
  g_test_bug_base ("https://bugzilla.redhat.com/show_bug.cgi?id=");

  // Define the tests.o

  g_test_add ("/modulemd/v2/modulestream/construct",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_test_construct,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/arch",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_test_arch,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/copy",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_test_copy,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/equals",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_test_equals,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/nsvc",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_test_nsvc,
              NULL);


  g_test_add ("/modulemd/v2/modulestream/nsvca",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_test_nsvca,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/v1/equals",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_v1_test_equals,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/v2/equals",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_v2_test_equals,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/v1/dependencies",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_v1_test_dependencies,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/v2/dependencies",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_v2_test_dependencies,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/v1/parse_dump",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_v1_test_parse_dump,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/v2/parse_dump",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_v2_test_parse_dump,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/v1/depends_on_stream",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_v1_test_depends_on_stream,
              NULL);
  g_test_add ("/modulemd/v2/modulestream/v2/depends_on_stream",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_v2_test_depends_on_stream,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/v2/validate/buildafter",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_v2_test_validate_buildafter,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/v2/rpm_map",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_v2_test_rpm_map,
              NULL);

  g_test_add_func ("/modulemd/v2/modulestream/v2/unicode/description",
                   module_stream_v2_test_unicode_desc);

  g_test_add_func ("/modulemd/v2/modulestream/v2/xmd/issue274",
                   module_stream_v2_test_xmd_issue_274);

  g_test_add_func ("/modulemd/v2/modulestream/v2/xmd/issue290",
                   module_stream_v2_test_xmd_issue_290);

  g_test_add_func ("/modulemd/v2/modulestream/v2/xmd/issue290plus",
                   module_stream_v2_test_xmd_issue_290_with_example);

  return g_test_run ();
}
