
#ifndef __ng_marshal_MARSHAL_H__
#define __ng_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* VOID:STRING,STRING,STRING (/home/mike/Projects/NekoGroup/src/ng-marshal.list:1) */
extern void ng_marshal_VOID__STRING_STRING_STRING (GClosure     *closure,
                                                   GValue       *return_value,
                                                   guint         n_param_values,
                                                   const GValue *param_values,
                                                   gpointer      invocation_hint,
                                                   gpointer      marshal_data);

/* VOID:STRING,STRING (/home/mike/Projects/NekoGroup/src/ng-marshal.list:2) */
extern void ng_marshal_VOID__STRING_STRING (GClosure     *closure,
                                            GValue       *return_value,
                                            guint         n_param_values,
                                            const GValue *param_values,
                                            gpointer      invocation_hint,
                                            gpointer      marshal_data);

G_END_DECLS

#endif /* __ng_marshal_MARSHAL_H__ */

