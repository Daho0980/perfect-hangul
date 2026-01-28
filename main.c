#include <ibus.h>
#include <hangul-1.0/hangul.h>

#include <stdio.h>


typedef struct _PerfectHangulEngine       PerfectHangulEngine    ;
typedef struct _PerfectHangulEngineClass PerfectHangulEngineClass;

struct _PerfectHangulEngine {
    IBusEngine          parent;
    HangulInputContext* hic   ;
};

struct _PerfectHangulEngineClass {
    IBusEngineClass parent;
};

static void perfect_hangul_engine_init(PerfectHangulEngine* engine);
static void perfect_hangul_engine_class_init(PerfectHangulEngineClass* klass);
GType perfect_hangul_engine_get_type(void);

static IBusEngineClass* parent_class = NULL;

#define TYPE_PERFECT_HANGUL (perfect_hangul_engine_get_type())
G_DEFINE_TYPE (PerfectHangulEngine, perfect_hangul_engine, IBUS_TYPE_ENGINE);

static void update_preedit(PerfectHangulEngine *engine) {
    const ucschar* ucsstr = hangul_ic_get_preedit_string(engine->hic);

    if ( ucsstr && *ucsstr ) {
        gchar* utf8str = g_ucs4_to_utf8(
            (const gunichar*)ucsstr, -1,
            NULL, NULL,
            NULL
        );
        if ( utf8str ) {
            IBusText* text = ibus_text_new_from_string(utf8str);
            guint     len  = g_utf8_strlen(utf8str, -1);

            ibus_text_append_attribute(
                text,
                IBUS_ATTR_TYPE_UNDERLINE,
                IBUS_ATTR_UNDERLINE_SINGLE,
                0,
                -1
            );

            ibus_engine_update_preedit_text(
                (IBusEngine*)engine,
                text, len,
                TRUE
            );

            g_object_unref(text);
            g_free(utf8str);
        }
    }
    else {
        ibus_engine_hide_preedit_text((IBusEngine*)engine);
    }
}

static void commit_string(PerfectHangulEngine* engine) {
    const ucschar* ucsstr = hangul_ic_get_commit_string(engine->hic);
    if ( ucsstr && *ucsstr ) {
        gchar* utf8str = g_ucs4_to_utf8(
            (const gunichar*)ucsstr, -1,
            NULL, NULL,
            NULL
        );
        if ( utf8str ) {
            IBusText* text = ibus_text_new_from_string(utf8str);
            ibus_engine_commit_text((IBusEngine*)engine, text);

            g_object_unref(text);
            g_free(utf8str);
        }
    }
}

static gboolean perfect_hangul_engine_process_key_event(
    IBusEngine* engine   ,
    guint       keyval   ,
    guint       keycode  ,
    guint       modifiers
) {
    PerfectHangulEngine* perfectHangulEngine = (PerfectHangulEngine*)engine;

    if ( modifiers & IBUS_RELEASE_MASK ) {
        return FALSE;
    }

    if ( modifiers & (IBUS_CONTROL_MASK|IBUS_MOD1_MASK) ) return FALSE;

    bool consumed = hangul_ic_process(perfectHangulEngine->hic, keyval);

    if ( consumed ) {
        commit_string(perfectHangulEngine);
        update_preedit(perfectHangulEngine);

        return TRUE;
    }

    if ( !hangul_ic_is_empty(perfectHangulEngine->hic) ) {
        hangul_ic_flush(perfectHangulEngine->hic);
        commit_string(perfectHangulEngine);
        update_preedit(perfectHangulEngine);
    }

    return FALSE;
}

static void perfect_hangul_engine_init(PerfectHangulEngine* engine) {
    engine->hic = hangul_ic_new("2");
}

static void perfect_hangul_engine_focus_in(IBusEngine* engine) {
    PerfectHangulEngine* perfectHangulEngine = (PerfectHangulEngine*)engine;

    hangul_ic_reset(perfectHangulEngine->hic);

    if ( parent_class && parent_class->focus_in ) {
        parent_class->focus_in(engine);
    }
}

static void perfect_hangul_engine_finalize(GObject* object) {
    PerfectHangulEngine* engine = (PerfectHangulEngine*)object;
    if ( engine->hic ) {
        hangul_ic_delete(engine->hic);
    }
    G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void perfect_hangul_engine_class_init(PerfectHangulEngineClass* klass) {
    IBusEngineClass* engineClass = IBUS_ENGINE_CLASS(klass);
    GObjectClass*    objectClass = G_OBJECT_CLASS(klass);

    parent_class = g_type_class_peek_parent(klass);

    engineClass->process_key_event = perfect_hangul_engine_process_key_event;
    engineClass->focus_in          = perfect_hangul_engine_focus_in         ;
    objectClass->finalize          = perfect_hangul_engine_finalize         ;
}

int main(int argc, char** argv){
    ibus_init();
    IBusBus* bus = ibus_bus_new();
    if ( !ibus_bus_is_connected(bus) ) {
        fprintf(stderr, "IBus bus not connected!\n");

        return 1;
    }

    ibus_bus_request_name(bus, "com.example.PerfectHangul", 0);

    IBusFactory* factory = ibus_factory_new(ibus_bus_get_connection(bus));
    ibus_factory_add_engine(factory, "perfect-hangul", TYPE_PERFECT_HANGUL);

    printf("\x1b[93mPerfect Hangul Engine\x1b[0m Started...\n");

    ibus_main();

    return 0;
}